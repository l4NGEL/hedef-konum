#include "hk/belirsizlik.hpp"

#include <algorithm>
#include <cmath>

namespace hk {

Belirsizlik ornek_belirsizlik() {
  Belirsizlik b;
  b.gps_yatay_m = 2.0;
  b.gps_dusey_m = 4.0;            // GPS'te düşey hata yataydan kötüdür
  b.roll_rad = derece2rad(0.2);
  b.pitch_rad = derece2rad(0.2);
  b.yaw_rad = derece2rad(0.5);    // pusula yönü: manyetometre, en zayıf açı
  b.pan_rad = derece2rad(0.1);
  b.tilt_rad = derece2rad(0.1);
  b.piksel = 2.0;
  b.dem_m = 5.0;
  return b;
}

Gozlem gozlemi_boz(const Gozlem& g, const Belirsizlik& b, std::mt19937& rng, double& dem_ofset) {
  std::normal_distribution<double> N(0.0, 1.0);
  Gozlem r = g;
  // GPS: konumu yerel NED'de metre olarak kaydırıp geri LLA'ya çeviriyoruz.
  // (Enlemi doğrudan "derece" olarak bozmak, metre karşılığını enleme göre
  //  değiştirir; NED'de bozmak her yerde aynı anlamı taşır.)
  const Vek3 kayma{b.gps_yatay_m * N(rng), b.gps_yatay_m * N(rng), b.gps_dusey_m * N(rng)};
  r.iha = ecef_to_lla(ned_to_ecef(kayma, g.iha));
  r.govde.roll_rad += b.roll_rad * N(rng);
  r.govde.pitch_rad += b.pitch_rad * N(rng);
  r.govde.yaw_rad += b.yaw_rad * N(rng);
  r.pan_rad += b.pan_rad * N(rng);
  r.tilt_rad += b.tilt_rad * N(rng);
  r.u += b.piksel * N(rng);
  r.v += b.piksel * N(rng);
  dem_ofset = b.dem_m * N(rng);
  return r;
}

double yuzdelik(std::vector<double> v, double p) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const double konum = p * (v.size() - 1);
  const size_t i = static_cast<size_t>(konum);
  if (i + 1 >= v.size()) return v.back();
  const double f = konum - i;
  return v[i] * (1 - f) + v[i + 1] * f;
}

McSonuc monte_carlo(const Kamera& k, const Gozlem& g, const Dem& dem, const Belirsizlik& b,
                    int n, uint32_t tohum, const DemAyar& ayar) {
  McSonuc m;
  m.n = n;
  const Sonuc gercek = geolokasyon(k, g, dem, ayar);
  if (!gercek.bulundu) { m.basarisiz = n; return m; }

  std::mt19937 rng(tohum);  // aynı tohum = aynı sonuç (tekrarlanabilir deney)
  std::vector<double> r;
  r.reserve(n);
  double sn = 0, se = 0, s2 = 0;
  for (int i = 0; i < n; ++i) {
    double ofset = 0;
    const Gozlem gb = gozlemi_boz(g, b, rng, ofset);
    const OfsetDem od(dem, ofset);
    const Sonuc s = geolokasyon(k, gb, od, ayar);
    if (!s.bulundu) { ++m.basarisiz; continue; }
    // Hatayı gerçek hedefin yerel NED düzleminde ölç: kuzey/doğu metre
    const Vek3 h = ecef_to_ned(lla_to_ecef(s.hedef), gercek.hedef);
    sn += h.x;
    se += h.y;
    s2 += h.x * h.x + h.y * h.y;
    r.push_back(std::hypot(h.x, h.y));
  }
  const double basarili = static_cast<double>(r.size());
  if (basarili > 0) {
    m.kuzey_bias_m = sn / basarili;
    m.dogu_bias_m = se / basarili;
    m.rms_m = std::sqrt(s2 / basarili);
    m.cep50_m = yuzdelik(r, 0.5);
    m.cep90_m = yuzdelik(r, 0.9);
  }
  return m;
}

std::vector<Katki> duyarlilik(const Kamera& k, const Gozlem& g, const Dem& dem,
                              const Belirsizlik& b, int n, uint32_t tohum, const DemAyar& ayar) {
  // Her seferinde tek kaynağı açık bırak, diğerlerini sıfırla.
  struct Kaynak { const char* ad; Belirsizlik bb; };
  std::vector<Kaynak> liste;
  auto ekle = [&](const char* ad, auto ayarla) { Belirsizlik x; ayarla(x); liste.push_back({ad, x}); };
  ekle("GPS yatay", [&](Belirsizlik& x) { x.gps_yatay_m = b.gps_yatay_m; });
  ekle("GPS dusey", [&](Belirsizlik& x) { x.gps_dusey_m = b.gps_dusey_m; });
  ekle("roll", [&](Belirsizlik& x) { x.roll_rad = b.roll_rad; });
  ekle("pitch", [&](Belirsizlik& x) { x.pitch_rad = b.pitch_rad; });
  ekle("yaw", [&](Belirsizlik& x) { x.yaw_rad = b.yaw_rad; });
  ekle("gimbal pan", [&](Belirsizlik& x) { x.pan_rad = b.pan_rad; });
  ekle("gimbal tilt", [&](Belirsizlik& x) { x.tilt_rad = b.tilt_rad; });
  ekle("piksel", [&](Belirsizlik& x) { x.piksel = b.piksel; });
  ekle("DEM", [&](Belirsizlik& x) { x.dem_m = b.dem_m; });

  std::vector<Katki> sonuc;
  for (const auto& kk : liste)
    sonuc.push_back({kk.ad, monte_carlo(k, g, dem, kk.bb, n, tohum, ayar)});
  sonuc.push_back({"HEPSI", monte_carlo(k, g, dem, b, n, tohum, ayar)});
  return sonuc;
}

}  // namespace hk

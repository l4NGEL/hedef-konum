#include "hk/senaryo.hpp"

#include <cmath>

namespace hk {

bool hedefi_izdusur(const Kamera& k, const LLA& iha, const Euler& govde, double pan,
                    double tilt, const LLA& hedef, double& u, double& v) {
  // Geri modelin tam tersi: ECEF -> NED -> kamera (R^T ile) -> piksel
  const Vek3 ned = ecef_to_ned(lla_to_ecef(hedef), iha);
  const Vek3 kam = R_ned_kam(govde, pan, tilt).devrik() * ned;
  if (!izdusur(k, kam, u, v)) return false;
  return u >= 0 && v >= 0 && u <= k.genislik - 1 && v <= k.yukseklik - 1;
}

void gimbali_yonelt(const LLA& iha, const Euler& govde, const LLA& hedef, double& pan,
                    double& tilt) {
  // Hedef yönünü gövde çerçevesine çevir, sonra Rz(pan)Ry(tilt)*x = d çöz:
  //   d = (cos t cos p, cos t sin p, -sin t)
  const Vek3 d_ned = birimle(ecef_to_ned(lla_to_ecef(hedef), iha));
  const Vek3 d = R_ned_govde(govde).devrik() * d_ned;
  pan = std::atan2(d.y, d.x);
  tilt = std::atan2(-d.z, std::hypot(d.x, d.y));
}

bool ucgenle(const std::vector<Isin>& isinlar, Vek3& p) {
  // Her ışın için P = I - d d^T, noktanın ışına dik sapmasını verir.
  // min Σ |P (p - o)|²  ->  (Σ P) p = Σ P o   (3x3 doğrusal sistem)
  if (isinlar.size() < 2) return false;
  Mat3 A;
  Vek3 b;
  for (const Isin& r : isinlar) {
    const Mat3 P = dik_izdusum(r.d);
    A = A + P;
    b = b + P * r.o;
  }
  // Işınlar neredeyse paralelse A tekile yaklaşır: çözüm güvenilmez.
  // det'i ölçekten bağımsız değerlendirmek için n³'e bölüyoruz.
  const double n = static_cast<double>(isinlar.size());
  if (std::abs(A.det()) / (n * n * n) < 1e-9) return false;
  p = A.ters() * b;
  return true;
}

Gozlem sabit_hata_ekle(const Gozlem& g, const Belirsizlik& s) {
  Gozlem r = g;
  r.iha = ecef_to_lla(ned_to_ecef({s.gps_yatay_m, 0.0, s.gps_dusey_m}, g.iha));
  r.govde.roll_rad += s.roll_rad;
  r.govde.pitch_rad += s.pitch_rad;
  r.govde.yaw_rad += s.yaw_rad;
  r.pan_rad += s.pan_rad;
  r.tilt_rad += s.tilt_rad;
  r.u += s.piksel;
  r.v += s.piksel;
  return r;
}

SenaryoSonuc senaryo_calistir(const Kamera& k, const Dem& dem, const SenaryoAyar& a) {
  SenaryoSonuc ss;
  ss.hedef = a.hedef;
  ss.hedef.h_m = dem.yukseklik(a.hedef.lat_rad, a.hedef.lon_rad);
  const double g0 = 9.80665;
  const double omega = a.hiz_mps / a.yaricap_m;               // açısal hız
  const double roll = std::atan(a.hiz_mps * a.hiz_mps / (g0 * a.yaricap_m));  // koordineli dönüş
  const OfsetDem dem_olcum(dem, a.sabit.dem_m);                // DEM yanlılığı

  std::mt19937 rng(a.tohum);
  std::vector<double> hatalar;
  std::vector<Isin> isinlar;
  Vek3 toplam;
  int n_ok = 0;

  for (double t = 0; t <= a.sure_s + 1e-9; t += a.dt_s) {
    // 1) İHA konumu: hedefin NED'inde daire. Açı kuzeyden doğuya artar.
    const double aci = omega * t;
    const Vek3 ned{a.yaricap_m * std::cos(aci), a.yaricap_m * std::sin(aci), -a.agl_m};
    Gozlem gercek;
    gercek.iha = ecef_to_lla(ned_to_ecef(ned, ss.hedef));
    // Yön: daireye teğet (saat yönü, yukarıdan bakınca); merkez sağ kanatta.
    gercek.govde = {roll, 0.0, aci + M_PI / 2};

    // 2) Gimbal hedefe döner ama takip mükemmel değil: küçük salınım
    gimbali_yonelt(gercek.iha, gercek.govde, ss.hedef, gercek.pan_rad, gercek.tilt_rad);
    const double sal = a.takip_salinim_piksel / k.fx;
    gercek.pan_rad += sal * std::sin(0.37 * t);
    gercek.tilt_rad += sal * std::cos(0.23 * t);

    // 3) İleri model: hedef hangi pikselde?
    if (!hedefi_izdusur(k, gercek.iha, gercek.govde, gercek.pan_rad, gercek.tilt_rad, ss.hedef,
                        gercek.u, gercek.v))
      continue;

    // 4) Ölçüm = gerçek + sabit yanlılık + rastgele gürültü
    double dem_rastgele = 0;
    Gozlem olcum = gozlemi_boz(sabit_hata_ekle(gercek, a.sabit), a.rastgele, rng, dem_rastgele);
    const OfsetDem dem_kare(dem_olcum, dem_rastgele);

    // 5) Geri model
    Kare kr;
    kr.t_s = t;
    kr.iha = gercek.iha;
    kr.yaw_rad = gercek.govde.yaw_rad;
    kr.u = olcum.u;
    kr.v = olcum.v;
    const Sonuc s = geolokasyon(k, olcum, dem_kare, a.dem);
    if (s.bulundu) {
      kr.bulundu = true;
      kr.tahmin = s.hedef;
      kr.hata_m = yatay_mesafe(ss.hedef, s.hedef);
      hatalar.push_back(kr.hata_m);
      toplam = toplam + lla_to_ecef(s.hedef);
      ++n_ok;
    }
    isinlar.push_back({lla_to_ecef(olcum.iha), isin_ecef(k, olcum)});
    ss.kareler.push_back(kr);
  }

  ss.kare_cep50_m = yuzdelik(hatalar, 0.5);
  ss.kare_cep90_m = yuzdelik(hatalar, 0.9);
  if (n_ok > 0) {
    ss.ortalama = ecef_to_lla((1.0 / n_ok) * toplam);
    ss.ortalama_hata_m = yatay_mesafe(ss.hedef, ss.ortalama);
  }
  Vek3 p;
  if (ucgenle(isinlar, p)) {
    ss.ucgen_ok = true;
    ss.ucgen = ecef_to_lla(p);
    ss.ucgen_hata_m = yatay_mesafe(ss.hedef, ss.ucgen);
  }
  return ss;
}

}  // namespace hk

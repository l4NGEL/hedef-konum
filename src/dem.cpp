#include "hk/dem.hpp"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace hk {

static const double NaN = std::numeric_limits<double>::quiet_NaN();

IzgaraDem::IzgaraDem(double kuzey, double bati, double adim, int satir, int sutun,
                     std::vector<float> veri)
    : kuzey_(kuzey), bati_(bati), adim_(adim), satir_(satir), sutun_(sutun), h_(std::move(veri)) {
  if (static_cast<long long>(h_.size()) != static_cast<long long>(satir) * sutun)
    throw std::invalid_argument("IzgaraDem: veri boyutu satir*sutun degil");
}

double IzgaraDem::yukseklik(double lat_rad, double lon_rad) const {
  // Kesirli satır/sütun konumu
  const double r = (kuzey_ - rad2derece(lat_rad)) / adim_;
  const double c = (rad2derece(lon_rad) - bati_) / adim_;
  if (!(r >= 0.0 && c >= 0.0 && r <= satir_ - 1 && c <= sutun_ - 1)) return NaN;

  // Çift doğrusal (bilinear) ara değer: çevredeki 4 hücrenin ağırlıklı ortalaması
  int r0 = static_cast<int>(r), c0 = static_cast<int>(c);
  if (r0 == satir_ - 1) --r0;  // tam alt/sağ kenarda dışarı taşmasın
  if (c0 == sutun_ - 1) --c0;
  const double fr = r - r0, fc = c - c0;
  auto H = [&](int i, int j) { return static_cast<double>(h_[static_cast<size_t>(i) * sutun_ + j]); };
  const double h00 = H(r0, c0), h01 = H(r0, c0 + 1), h10 = H(r0 + 1, c0), h11 = H(r0 + 1, c0 + 1);
  // NaN (veri yok) herhangi bir köşedeyse sonuç da NaN olur: bilerek.
  return (1 - fr) * ((1 - fc) * h00 + fc * h01) + fr * ((1 - fc) * h10 + fc * h11);
}

IzgaraDem IzgaraDem::hgt_oku(const std::string& yol) {
  // Dosya adından sol alt (güney-batı) köşe: N40E032 -> 40N, 32E
  const size_t bas = yol.find_last_of("/\\") == std::string::npos ? 0 : yol.find_last_of("/\\") + 1;
  const std::string ad = yol.substr(bas);
  if (ad.size() < 7) throw std::runtime_error("hgt: dosya adi beklenen bicimde degil: " + ad);
  const int lat = std::stoi(ad.substr(1, 2)) * (ad[0] == 'S' || ad[0] == 's' ? -1 : 1);
  const int lon = std::stoi(ad.substr(4, 3)) * (ad[3] == 'W' || ad[3] == 'w' ? -1 : 1);

  std::ifstream f(yol, std::ios::binary | std::ios::ate);
  if (!f) throw std::runtime_error("hgt: acilamadi: " + yol);
  const auto bayt = static_cast<long long>(f.tellg());
  int n = 0;
  if (bayt == 1201LL * 1201 * 2) n = 1201;
  else if (bayt == 3601LL * 3601 * 2) n = 3601;
  else throw std::runtime_error("hgt: beklenmeyen boyut");
  f.seekg(0);

  std::vector<unsigned char> ham(static_cast<size_t>(bayt));
  f.read(reinterpret_cast<char*>(ham.data()), bayt);
  std::vector<float> h(static_cast<size_t>(n) * n);
  for (size_t i = 0; i < h.size(); ++i) {
    // Büyük-endian: önce yüksek bayt. int16'ya çevirmeden önce birleştir.
    const auto v = static_cast<int16_t>((ham[2 * i] << 8) | ham[2 * i + 1]);
    h[i] = (v == -32768) ? std::numeric_limits<float>::quiet_NaN() : static_cast<float>(v);
  }
  // Karolar 1 derece; kenar satır/sütun komşu karoyla ortaktır (bu yüzden n-1 aralık)
  return IzgaraDem(lat + 1.0, lon, 1.0 / (n - 1), n, n, std::move(h));
}

// Işın üzerindeki noktanın zeminden yüksekliği (pozitif = zeminin üstünde)
static double zemin_ustu(const Dem& dem, const Vek3& p) {
  const LLA q = ecef_to_lla(p);
  return q.h_m - dem.yukseklik(q.lat_rad, q.lon_rad);
}

Sonuc dem_isin_yurut(const Dem& dem, const Vek3& o, const Vek3& d, const DemAyar& ayar) {
  Sonuc s;
  double f_once = zemin_ustu(dem, o);
  if (!(f_once > 0.0)) return s;  // NaN ya da İHA zeminin altında: anlamsız

  double t_once = 0.0;
  for (double t = ayar.adim_m; t <= ayar.max_menzil_m; t += ayar.adim_m) {
    const double f = zemin_ustu(dem, o + t * d);
    ++s.adim;
    if (std::isnan(f)) return s;  // DEM dışına çıktık
    if (f <= 0.0) {
      // [t_once, t] aralığında işaret değişti: ikiye bölme (bisection)
      double lo = t_once, hi = t;
      for (int i = 0; i < 50 && hi - lo > 1e-4; ++i) {
        const double orta = 0.5 * (lo + hi);
        if (zemin_ustu(dem, o + orta * d) > 0.0) lo = orta; else hi = orta;
        ++s.adim;
      }
      s.bulundu = true;
      s.menzil_m = 0.5 * (lo + hi);
      s.hedef = ecef_to_lla(o + s.menzil_m * d);
      return s;
    }
    t_once = t;
    f_once = f;
  }
  return s;  // menzil içinde zemine değmedi
}

Sonuc dem_yinelemeli(const Dem& dem, const Vek3& o, const Vek3& d, double h0, int max_iter) {
  double h = h0;
  Sonuc s;
  for (int i = 0; i < max_iter; ++i) {
    s = elipsoid_kesisim(o, d, h);
    s.adim = i + 1;
    if (!s.bulundu) return s;
    const double h_yeni = dem.yukseklik(s.hedef.lat_rad, s.hedef.lon_rad);
    if (std::isnan(h_yeni)) { s.bulundu = false; return s; }
    if (std::abs(h_yeni - h) < 0.01) return s;  // yakınsadı
    h = h_yeni;
  }
  s.bulundu = false;  // yakınsamadı (salınım)
  return s;
}

Sonuc geolokasyon(const Kamera& k, const Gozlem& g, const Dem& dem, const DemAyar& ayar) {
  return dem_isin_yurut(dem, lla_to_ecef(g.iha), isin_ecef(k, g), ayar);
}

}  // namespace hk

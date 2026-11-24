#pragma once
// Sayısal Yükseklik Modeli (DEM) ve ışın yürütme.
//
// DEM, enlem/boylam ızgarasında zeminin yüksekliğini tutar. SRTM gibi
// açık modeller ~30 m çözünürlükte, ~5-10 m mutlak doğrulukta.
//
// DİKKAT (yükseklik referansı): SRTM yükseklikleri EGM96 jeoidine (≈ deniz
// seviyesi) göredir, GPS'in h'si ise WGS84 elipsoidine. Türkiye'de fark
// ~+30..+40 m. Bu projede DEM'in elipsoid yüksekliği verdiğini varsayıyoruz;
// gerçek SRTM ile çalışırken jeoid ondülasyonunu eklemek gerekir (docs/05_dem.md).
#include <functional>
#include <string>
#include <vector>

#include "hk/geolokasyon.hpp"

namespace hk {

class Dem {
 public:
  virtual ~Dem() = default;
  // Elipsoid yüksekliği [m]. Veri yoksa NaN.
  virtual double yukseklik(double lat_rad, double lon_rad) const = 0;
};

class SabitDem : public Dem {
 public:
  explicit SabitDem(double h) : h_(h) {}
  double yukseklik(double, double) const override { return h_; }

 private:
  double h_;
};

// Testler ve sentetik arazi için: f(enlem_derece, boylam_derece) -> yükseklik
class FonksiyonDem : public Dem {
 public:
  explicit FonksiyonDem(std::function<double(double, double)> f) : f_(std::move(f)) {}
  double yukseklik(double lat, double lon) const override {
    return f_(rad2derece(lat), rad2derece(lon));
  }

 private:
  std::function<double(double, double)> f_;
};

// Bir DEM'e sabit hata ekler (Monte Carlo'da DEM yanlılığı için).
class OfsetDem : public Dem {
 public:
  OfsetDem(const Dem& alt, double ofset) : alt_(alt), ofset_(ofset) {}
  double yukseklik(double lat, double lon) const override {
    return alt_.yukseklik(lat, lon) + ofset_;
  }

 private:
  const Dem& alt_;
  double ofset_;
};

// Düzenli enlem/boylam ızgarası. Satır 0 = en kuzey (SRTM .hgt düzeni).
class IzgaraDem : public Dem {
 public:
  IzgaraDem(double kuzey_lat_deg, double bati_lon_deg, double adim_deg, int satir, int sutun,
            std::vector<float> veri);
  double yukseklik(double lat_rad, double lon_rad) const override;

  // SRTM .hgt karosu: dosya adı N40E032.hgt gibi; 1201x1201 (3") veya
  // 3601x3601 (1"); büyük-endian int16; -32768 = veri yok.
  static IzgaraDem hgt_oku(const std::string& yol);

  int satir() const { return satir_; }
  int sutun() const { return sutun_; }

 private:
  double kuzey_, bati_, adim_;
  int satir_, sutun_;
  std::vector<float> h_;
};

struct DemAyar {
  double adim_m{10.0};          // ışın yürütme adımı
  double max_menzil_m{30000.0}; // bu kadar gidip bir şeye çarpmazsak vazgeç
};

// Yöntem A: ışın yürütme (ray marching). Işın boyunca küçük adımlarla ilerle,
// zeminin altına ilk geçtiğin aralığı ikiye bölerek daralt. Tepenin arkasında
// kalan noktaları doğru şekilde "görmez" (örtülme / occlusion).
Sonuc dem_isin_yurut(const Dem& dem, const Vek3& o, const Vek3& d, const DemAyar& ayar = {});

// Yöntem B: yinelemeli düz yükseklik. h0 ile elipsoid kesişimi bul, oradaki
// DEM yüksekliğini oku, onunla tekrar kesiştir... Hızlıdır, yumuşak arazide
// iyi çalışır; dik arazide salınır, örtülmeyi hiç anlamaz.
Sonuc dem_yinelemeli(const Dem& dem, const Vek3& o, const Vek3& d, double h0, int max_iter = 30);

// Hepsi bir arada: gözlem + DEM -> hedef (yöntem A)
Sonuc geolokasyon(const Kamera& k, const Gozlem& g, const Dem& dem, const DemAyar& ayar = {});

}  // namespace hk

#pragma once
// Hata bütçesi. "Hedef burada" demek yetmez; "%50 olasılıkla X m,
// %90 olasılıkla Y m içinde" demek gerekir. Bunu Monte Carlo ile buluyoruz:
// her girdiyi kendi hatası kadar rastgele boz, binlerce kez çöz, dağılıma bak.
//
//   CEP50 (Circular Error Probable): sonuçların yarısının içine düştüğü çember
//   CEP90: %90'ının içine düştüğü çember
//   Yanlılık (bias): ortalama hatanın kendisi (hep aynı yöne kayma)
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "hk/dem.hpp"

namespace hk {

// 1-sigma hatalar. Hepsi sıfırsa girdi kusursuz demektir.
struct Belirsizlik {
  double gps_yatay_m{0};  // her yatay eksende
  double gps_dusey_m{0};
  double roll_rad{0}, pitch_rad{0}, yaw_rad{0};  // IMU / AHRS
  double pan_rad{0}, tilt_rad{0};                // gimbal enkoderleri
  double piksel{0};                              // tespit kutusu merkezi (her eksen)
  double dem_m{0};                               // DEM yükseklik hatası
};

// Örnek değerler: bu projede varsayılan "taktik İHA" hata bütçesi.
// Kaynak değil, makul varsayım; README'de böyle belirtiliyor.
Belirsizlik ornek_belirsizlik();

// Gözlemi verilen sigmalarla bir kez rastgele bozar. dem_ofset_m çıktısı,
// bu örnek için DEM'e eklenecek hata.
Gozlem gozlemi_boz(const Gozlem& g, const Belirsizlik& b, std::mt19937& rng,
                   double& dem_ofset_m);

struct McSonuc {
  int n{0}, basarisiz{0};
  double kuzey_bias_m{0}, dogu_bias_m{0};  // ortalama hata vektörü
  double cep50_m{0}, cep90_m{0}, rms_m{0};
};

// Gözlemin kusursuz çözümünü "gerçek" kabul edip n örnekle dağılımı ölçer.
McSonuc monte_carlo(const Kamera& k, const Gozlem& g, const Dem& dem, const Belirsizlik& b,
                    int n, uint32_t tohum, const DemAyar& ayar = {});

// Hata kaynaklarını tek tek açarak katkılarını çıkarır (duyarlılık analizi).
struct Katki {
  std::string kaynak;
  McSonuc sonuc;
};
std::vector<Katki> duyarlilik(const Kamera& k, const Gozlem& g, const Dem& dem,
                              const Belirsizlik& b, int n, uint32_t tohum,
                              const DemAyar& ayar = {});

// Sıralanmış değerlerden yüzdelik (doğrusal ara değerli)
double yuzdelik(std::vector<double> v, double p);

}  // namespace hk

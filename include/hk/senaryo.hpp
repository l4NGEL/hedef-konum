#pragma once
// Doğrulama. Gerçek uçuş verimiz yok; bu yüzden gerçeği bildiğimiz bir
// dünya kuruyoruz:
//   1) Hedefi DEM üzerinde bilinen bir yere koy.
//   2) İHA'yı onun etrafında daire çizdir, gimbal'i hedefe çevir.
//   3) İLERİ MODEL: hedefin görüntüde hangi piksele düşeceğini hesapla.
//   4) Telemetriye ve piksele hata ekle (rastgele + sabit yanlılık).
//   5) GERİ MODEL (geolocation) ile hedefi bul, gerçekle karşılaştır.
// İleri ve geri model aynı dönüşleri kullandığı için, hatasız durumda sonuç
// milimetre düzeyinde gerçekle örtüşmeli. Örtüşmüyorsa bir yerde hata var.
#include <cstdint>
#include <vector>

#include "hk/belirsizlik.hpp"

namespace hk {

// İleri model: hedef görüntüde nereye düşer? (görüş alanı dışındaysa false)
bool hedefi_izdusur(const Kamera& k, const LLA& iha, const Euler& govde, double pan,
                    double tilt, const LLA& hedef, double& u, double& v);

// Gimbal'in hedefe bakması için gereken pan/tilt (gövdeye göre)
void gimbali_yonelt(const LLA& iha, const Euler& govde, const LLA& hedef, double& pan,
                    double& tilt);

// Çok bakışlı üçgenleme: ışınlara dik uzaklık kareleri toplamını en aza indiren
// nokta. DEM gerektirmez; ama ışınlar birbirine paralelse çözüm kötüleşir.
struct Isin {
  Vek3 o, d;  // ECEF başlangıç, birim yön
};
bool ucgenle(const std::vector<Isin>& isinlar, Vek3& p_ecef);

// Sabit (her karede aynı) hatayı gözleme ekler. GPS yanlılığı kuzeye
// gps_yatay_m, aşağıya gps_dusey_m olarak uygulanır.
Gozlem sabit_hata_ekle(const Gozlem& g, const Belirsizlik& sabit);

struct SenaryoAyar {
  LLA hedef;                       // h_m kullanılmaz, DEM'den alınır
  double yaricap_m{1500};          // yörünge yarıçapı
  double agl_m{800};               // hedefin zemininden yükseklik
  double hiz_mps{25};
  double sure_s{120};
  double dt_s{1.0};
  double takip_salinim_piksel{40}; // gimbal hedefi tam merkezde tutamıyor
  Belirsizlik rastgele;            // her karede yeniden çekilen hata
  Belirsizlik sabit;               // tüm uçuş boyunca sabit kalan yanlılık
  uint32_t tohum{1};
  DemAyar dem;
};

struct Kare {
  double t_s{0};
  LLA iha;             // gerçek konum
  double yaw_rad{0};
  double u{0}, v{0};   // ölçülen piksel
  bool bulundu{false};
  LLA tahmin;
  double hata_m{0};    // yatay hata
};

struct SenaryoSonuc {
  LLA hedef;                       // gerçek hedef (DEM yüksekliğiyle)
  std::vector<Kare> kareler;
  double kare_cep50_m{0}, kare_cep90_m{0};
  LLA ortalama;                    // karelerin ortalaması
  double ortalama_hata_m{0};
  bool ucgen_ok{false};
  LLA ucgen;                       // çok bakışlı üçgenleme
  double ucgen_hata_m{0};
};

SenaryoSonuc senaryo_calistir(const Kamera& k, const Dem& dem, const SenaryoAyar& a);

}  // namespace hk

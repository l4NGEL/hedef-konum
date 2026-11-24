// ileri model, üçgenleme, yörünge senaryosu, çıktılar
#include <gtest/gtest.h>

#include <algorithm>

#include "../app/sentetik_arazi.hpp"
#include "hk/harita.hpp"

using namespace hk;

static Kamera kam() { return kamera_fovdan(1280, 720, derece2rad(30)); }

TEST(Senaryo7, GimbalHedefiMerkezePutar) {
  LLA hedef = lla_derece(40, 33, 1000);
  LLA iha = ecef_to_lla(ned_to_ecef({700, -1200, -900}, hedef));
  Euler govde = euler_derece(8, -3, 210);
  double pan, tilt, u, v;
  gimbali_yonelt(iha, govde, hedef, pan, tilt);
  ASSERT_TRUE(hedefi_izdusur(kam(), iha, govde, pan, tilt, hedef, u, v));
  EXPECT_NEAR(u, kam().cx, 1e-6);
  EXPECT_NEAR(v, kam().cy, 1e-6);
}

TEST(Senaryo7, IleriGeriModelTutarli) {
  // Rastgele bir pikselde görünen noktayı geri çözünce aynı nokta çıkmalı.
  FonksiyonDem dem = sentetik_arazi();
  Gozlem g;
  LLA merkez = lla_derece(ARAZI_LAT0, ARAZI_LON0, 0);
  merkez.h_m = dem.yukseklik(merkez.lat_rad, merkez.lon_rad);
  g.iha = ecef_to_lla(ned_to_ecef({-1500, 400, -800}, merkez));
  g.govde = euler_derece(12, 2, 15);
  gimbali_yonelt(g.iha, g.govde, merkez, g.pan_rad, g.tilt_rad);
  g.u = 1000;
  g.v = 150;
  Sonuc s = geolokasyon(kam(), g, dem, {1.0, 30000});
  ASSERT_TRUE(s.bulundu);
  double u, v;
  ASSERT_TRUE(hedefi_izdusur(kam(), g.iha, g.govde, g.pan_rad, g.tilt_rad, s.hedef, u, v));
  EXPECT_NEAR(u, 1000, 1e-3);
  EXPECT_NEAR(v, 150, 1e-3);
}

TEST(Senaryo7, UcgenlemeKesisenIsinlar) {
  const Vek3 p{4e6, 3e6, 2e6};
  std::vector<Isin> r;
  for (Vek3 o : {Vek3{4e6 + 1000, 3e6, 2e6}, Vek3{4e6, 3e6 + 1500, 2e6 + 300},
                 Vek3{4e6 - 800, 3e6 - 900, 2e6 + 1000}})
    r.push_back({o, birimle(p - o)});
  Vek3 q;
  ASSERT_TRUE(ucgenle(r, q));
  EXPECT_LT(norm(q - p), 1e-4);
}

TEST(Senaryo7, UcgenlemeParalelIsinlardaReddeder) {
  std::vector<Isin> r = {{{0, 0, 0}, {1, 0, 0}}, {{0, 1, 0}, {1, 0, 0}}};
  Vek3 q;
  EXPECT_FALSE(ucgenle(r, q));
}

TEST(Senaryo7, HatasizSenaryoMilimetre) {
  SenaryoAyar a;
  a.hedef = lla_derece(ARAZI_LAT0, ARAZI_LON0, 0);
  a.sure_s = 60;
  a.dem.adim_m = 2.0;
  SenaryoSonuc s = senaryo_calistir(kam(), sentetik_arazi(), a);
  ASSERT_GT(s.kareler.size(), 50u);
  EXPECT_LT(s.kare_cep90_m, 0.01);
  ASSERT_TRUE(s.ucgen_ok);
  EXPECT_LT(s.ucgen_hata_m, 0.01);
}

static SenaryoSonuc yaw_yanlilikli_tur(const Dem& dem) {
  SenaryoAyar a;
  a.hedef = lla_derece(ARAZI_LAT0, ARAZI_LON0, 0);
  a.sure_s = 377;  // tam tur
  a.dt_s = 2;
  a.sabit.yaw_rad = derece2rad(1.0);
  return senaryo_calistir(kam(), dem, a);
}

TEST(Senaryo7, DuzZemindeTamTurYawYanliliginiSondurur) {
  // Sabit yaw hatası her karede hedefi İHA'ya göre yana kaydırır; İHA hedefin
  // etrafında döndükçe kayma yönü de döner ve ortalamada söner.
  SenaryoSonuc s = yaw_yanlilikli_tur(SabitDem(1000));
  EXPECT_GT(s.kare_cep50_m, 20.0);                    // tek kare: ~1500 m × 1° ≈ 26 m
  EXPECT_LT(s.ortalama_hata_m, s.kare_cep50_m / 20);  // ortalama: neredeyse sıfır
}

TEST(Senaryo7, TepelikAraziYawYanliliginiTamSondurmez) {
  // Tepelik arazide yana kayan nokta başka yükseklikte bir zemine düşer; bu
  // da bakış yönünde ek bir kayma yaratır. Bu ikinci kayma arazinin şekline
  // bağlıdır ve tur boyunca sönmez: ortalama iyileşir ama sıfıra gitmez.
  SenaryoSonuc duz = yaw_yanlilikli_tur(SabitDem(1000));
  SenaryoSonuc tepe = yaw_yanlilikli_tur(sentetik_arazi());
  EXPECT_LT(tepe.ortalama_hata_m, tepe.kare_cep50_m / 2);
  EXPECT_GT(tepe.ortalama_hata_m, 5 * duz.ortalama_hata_m);
}

TEST(Senaryo8, CsvVeHtml) {
  SenaryoAyar a;
  a.hedef = lla_derece(ARAZI_LAT0, ARAZI_LON0, 0);
  a.sure_s = 10;
  SenaryoSonuc s = senaryo_calistir(kam(), sentetik_arazi(), a);
  const std::string csv = kareler_csv(s);
  EXPECT_EQ(std::count(csv.begin(), csv.end(), '\n'), static_cast<long>(s.kareler.size()) + 1);
  const std::string html = harita_html("deneme", {{"a", "#ff0000", &s}});
  EXPECT_NE(html.find("leaflet@1.9.4"), std::string::npos);
  EXPECT_NE(html.find("\"cep50\":"), std::string::npos);
}

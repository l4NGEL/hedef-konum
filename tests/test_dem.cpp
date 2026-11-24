// DEM ve ışın yürütme
#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "../app/sentetik_arazi.hpp"
#include "hk/dem.hpp"
#include "hk/senaryo.hpp"

using namespace hk;

static Kamera kam() { return kamera_fovdan(1280, 720, derece2rad(30)); }

static Gozlem hedefe_bak(const LLA& iha, const LLA& hedef) {
  Gozlem g;
  g.iha = iha;
  g.govde = euler_derece(0, 0, 0);
  gimbali_yonelt(iha, g.govde, hedef, g.pan_rad, g.tilt_rad);
  const Kamera k = kam();
  g.u = k.cx;
  g.v = k.cy;
  return g;
}

TEST(Dem5, SabitDemElipsoidleAyni) {
  SabitDem dem(900);
  Gozlem g;
  g.iha = lla_derece(40, 33, 2000);
  g.tilt_rad = derece2rad(-30);
  g.u = kam().cx;
  g.v = kam().cy;
  Sonuc a = geolokasyon(kam(), g, dem), b = elipsoid(kam(), g, 900);
  ASSERT_TRUE(a.bulundu && b.bulundu);
  EXPECT_LT(yatay_mesafe(a.hedef, b.hedef), 0.01);
}

TEST(Dem5, IzgaraBilinear) {
  // 2x2 ızgara: kuzeyde 0 ve 10, güneyde 20 ve 30. Tam ortada 15 olmalı.
  IzgaraDem d(41.0, 33.0, 1.0, 2, 2, {0, 10, 20, 30});
  EXPECT_NEAR(d.yukseklik(derece2rad(40.5), derece2rad(33.5)), 15.0, 1e-9);
  EXPECT_NEAR(d.yukseklik(derece2rad(41.0), derece2rad(34.0)), 10.0, 1e-9);
  EXPECT_TRUE(std::isnan(d.yukseklik(derece2rad(39.9), derece2rad(33.5))));  // dışarıda
}

TEST(Dem5, HgtOkuma) {
  // 1201x1201 sentetik .hgt yaz: değer = satır (büyük-endian), bir hücre boş.
  namespace fs = std::filesystem;
  const fs::path klasor = fs::temp_directory_path() / "hk_test_hgt";
  fs::create_directories(klasor);
  const fs::path yol = klasor / "N40E032.hgt";
  {
    std::ofstream f(yol, std::ios::binary);
    for (int r = 0; r < 1201; ++r)
      for (int c = 0; c < 1201; ++c) {
        int16_t v = (r == 600 && c == 600) ? static_cast<int16_t>(-32768) : static_cast<int16_t>(r);
        const unsigned char b[2] = {static_cast<unsigned char>((v >> 8) & 0xFF),
                                    static_cast<unsigned char>(v & 0xFF)};
        f.write(reinterpret_cast<const char*>(b), 2);
      }
  }
  IzgaraDem d = IzgaraDem::hgt_oku(yol.string());
  EXPECT_EQ(d.satir(), 1201);
  // Kuzey kenarı (41°) satır 0 -> 0 m; güney kenarı (40°) satır 1200 -> 1200 m
  EXPECT_NEAR(d.yukseklik(derece2rad(41.0), derece2rad(32.3)), 0.0, 1e-6);
  EXPECT_NEAR(d.yukseklik(derece2rad(40.0), derece2rad(32.3)), 1200.0, 1e-6);
  EXPECT_NEAR(d.yukseklik(derece2rad(40.75), derece2rad(32.3)), 300.0, 1e-6);
  EXPECT_TRUE(std::isnan(d.yukseklik(derece2rad(40.5), derece2rad(32.5))));  // boş hücre
  fs::remove_all(klasor);
}

TEST(Dem5, TepelikAzaraziGidisDonus) {
  // İleri model + geri model: gerçek hedefe bakan merkez pikseli, DEM ile
  // çözünce aynı noktayı vermeli.
  FonksiyonDem dem = sentetik_arazi();
  LLA hedef = lla_derece(ARAZI_LAT0 + 0.004, ARAZI_LON0 + 0.003, 0);
  hedef.h_m = dem.yukseklik(hedef.lat_rad, hedef.lon_rad);
  LLA iha = ecef_to_lla(ned_to_ecef({-2500, -800, -900}, hedef));
  Sonuc s = geolokasyon(kam(), hedefe_bak(iha, hedef), dem, {1.0, 30000});
  ASSERT_TRUE(s.bulundu);
  EXPECT_LT(yatay_mesafe(hedef, s.hedef), 0.05);
}

TEST(Dem5, TepeArkasiOrtulme) {
  // Kameranın önünde 300 m'lik bir sırt, hedef sırtın arkasında ve ışın
  // sırta çarpıyor. Işın yürütme sırtı bulur (fiziksel doğru cevap: hedef
  // görünmüyor, görülen şey sırt); yinelemeli yöntem sırtı "atlar".
  const double lat0 = 40.0;
  FonksiyonDem dem([&](double lat, double) {
    const double dn = (lat - lat0) * 111000.0;
    return 1000.0 + 300.0 * std::exp(-std::pow((dn - 2000.0) / 150.0, 2));  // 2 km kuzeyde sırt
  });
  LLA iha = lla_derece(lat0, 33.0, 1250);  // sırttan alçakta
  Gozlem g;
  g.iha = iha;
  g.tilt_rad = derece2rad(-4.0);
  g.u = kam().cx;
  g.v = kam().cy;
  const Vek3 o = lla_to_ecef(iha), d = isin_ecef(kam(), g);

  Sonuc a = dem_isin_yurut(dem, o, d, {2.0, 30000});
  ASSERT_TRUE(a.bulundu);
  const double kuzey_a = ecef_to_ned(lla_to_ecef(a.hedef), iha).x;
  EXPECT_GT(kuzey_a, 1500);
  EXPECT_LT(kuzey_a, 2000);  // sırtın ön yüzü

  Sonuc b = dem_yinelemeli(dem, o, d, 1000.0);
  ASSERT_TRUE(b.bulundu);
  const double kuzey_b = ecef_to_ned(lla_to_ecef(b.hedef), iha).x;
  EXPECT_GT(kuzey_b, 3000);  // sırtın ötesindeki düzlüğe "düştü": yanlış
}

TEST(Dem5, BuyukAdimInceSirtiKacirir) {
  // Adım, sırt kalınlığından (~30 m) büyükse ışın sırtın içinden "geçip"
  // gidebilir. "Gidebilir": sonuç örneklerin nereye düştüğüne bağlı. 100 m
  // adımda bir örnek tesadüfen sırtın tam üstüne (≈1995 m) düşüp yakalıyor;
  // 70 m adımda örnekler 1955 ve 2025 m'ye düşüp sırtı ıskalıyor (örtüşme).
  const double lat0 = 40.0;
  FonksiyonDem dem([&](double lat, double) {
    const double dn = (lat - lat0) * 111000.0;
    return 1000.0 + 300.0 * std::exp(-std::pow((dn - 2000.0) / 15.0, 2));  // ince sırt
  });
  LLA iha = lla_derece(lat0, 33.0, 1250);
  Gozlem g;
  g.iha = iha;
  g.tilt_rad = derece2rad(-4.0);
  g.u = kam().cx;
  g.v = kam().cy;
  const Vek3 o = lla_to_ecef(iha), d = isin_ecef(kam(), g);
  Sonuc ince = dem_isin_yurut(dem, o, d, {1.0, 30000});
  Sonuc kaba = dem_isin_yurut(dem, o, d, {70.0, 30000});
  ASSERT_TRUE(ince.bulundu && kaba.bulundu);
  EXPECT_LT(ecef_to_ned(lla_to_ecef(ince.hedef), iha).x, 2000);
  EXPECT_GT(ecef_to_ned(lla_to_ecef(kaba.hedef), iha).x, 3000);
}

TEST(Dem5, DemDisindaBulunmaz) {
  IzgaraDem d(41.0, 33.0, 0.01, 3, 3, std::vector<float>(9, 500.f));
  Gozlem g;
  g.iha = lla_derece(40.995, 33.005, 1500);
  g.tilt_rad = derece2rad(-1);  // çok sığ: DEM karosundan dışarı çıkar
  g.u = kam().cx;
  g.v = kam().cy;
  EXPECT_FALSE(geolokasyon(kam(), g, d).bulundu);
}

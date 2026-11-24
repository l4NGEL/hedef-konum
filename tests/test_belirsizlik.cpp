// Monte Carlo ve duyarlılık
#include <gtest/gtest.h>

#include "hk/belirsizlik.hpp"
#include "hk/senaryo.hpp"

using namespace hk;

static Kamera kam() { return kamera_fovdan(1280, 720, derece2rad(30)); }

// Düz zeminde, hedefin 'yatay' m güneyinden 'agl' m yukarıdan bakış
static Gozlem bakis(double agl, double yatay) {
  LLA hedef = lla_derece(40, 33, 1000);
  Gozlem g;
  g.iha = ecef_to_lla(ned_to_ecef({-yatay, 0, -agl}, hedef));
  g.govde = euler_derece(0, 0, 0);
  gimbali_yonelt(g.iha, g.govde, hedef, g.pan_rad, g.tilt_rad);
  g.u = kam().cx;
  g.v = kam().cy;
  return g;
}

TEST(Mc6, Yuzdelik) {
  EXPECT_DOUBLE_EQ(yuzdelik({3, 1, 2}, 0.5), 2.0);
  EXPECT_DOUBLE_EQ(yuzdelik({0, 10}, 0.9), 9.0);
}

TEST(Mc6, HatasizDagilimSifir) {
  SabitDem dem(1000);
  McSonuc m = monte_carlo(kam(), bakis(800, 1000), dem, Belirsizlik{}, 200, 1);
  EXPECT_EQ(m.basarisiz, 0);
  EXPECT_LT(m.cep90_m, 0.01);
}

TEST(Mc6, TekrarlanabilirTohum) {
  SabitDem dem(1000);
  McSonuc a = monte_carlo(kam(), bakis(800, 1000), dem, ornek_belirsizlik(), 300, 7);
  McSonuc b = monte_carlo(kam(), bakis(800, 1000), dem, ornek_belirsizlik(), 300, 7);
  EXPECT_DOUBLE_EQ(a.cep50_m, b.cep50_m);
}

TEST(Mc6, AciHatasiIkiKatinaCikincaCepIkiKat) {
  // Küçük açılarda hata doğrusal: sigma 2 kat -> CEP ~2 kat
  SabitDem dem(1000);
  Belirsizlik b1, b2;
  b1.tilt_rad = derece2rad(0.1);
  b2.tilt_rad = derece2rad(0.2);
  const Gozlem g = bakis(800, 1500);
  const double c1 = monte_carlo(kam(), g, dem, b1, 2000, 3).cep50_m;
  const double c2 = monte_carlo(kam(), g, dem, b2, 2000, 3).cep50_m;
  EXPECT_NEAR(c2 / c1, 2.0, 0.15);
}

TEST(Mc6, EgikBakisDikBakistanKotu) {
  SabitDem dem(1000);
  const Belirsizlik b = ornek_belirsizlik();
  const double dik = monte_carlo(kam(), bakis(800, 200), dem, b, 1000, 5).cep50_m;
  const double egik = monte_carlo(kam(), bakis(800, 3000), dem, b, 1000, 5).cep50_m;
  EXPECT_GT(egik, 3 * dik);
}

TEST(Mc6, NadirdeYawHatasiEtkisiz) {
  // Dümdüz aşağı bakarken merkez piksel dikey eksen üzerinde: yaw onu
  // kendi etrafında döndürür, yer değiştirmez.
  SabitDem dem(1000);
  Gozlem g;
  g.iha = lla_derece(40, 33, 1800);
  g.tilt_rad = derece2rad(-90);
  g.u = kam().cx;
  g.v = kam().cy;
  Belirsizlik b;
  b.yaw_rad = derece2rad(2.0);  // büyük yaw hatası
  EXPECT_LT(monte_carlo(kam(), g, dem, b, 500, 1).cep90_m, 0.01);
}

TEST(Mc6, EgikBakistaDemHatasiMenzilYonunde) {
  // DEM hatası hedefi bakış yönünde (kuzey) ileri-geri kaydırır, yana değil.
  SabitDem dem(1000);
  Belirsizlik b;
  b.dem_m = 10;
  McSonuc m = monte_carlo(kam(), bakis(500, 3000), dem, b, 1000, 9);
  // tan(bakış açısı) = 500/3000 -> 10 m DEM hatası ~60 m menzil kayması
  EXPECT_NEAR(m.cep50_m, 0.674 * 60.0, 8.0);  // yarı-normal medyanı 0,674 sigma
}

TEST(Mc6, DuyarlilikHepsiEnBuyuk) {
  SabitDem dem(1000);
  auto liste = duyarlilik(kam(), bakis(800, 1500), dem, ornek_belirsizlik(), 500, 11);
  ASSERT_EQ(liste.back().kaynak, "HEPSI");
  for (size_t i = 0; i + 1 < liste.size(); ++i)
    EXPECT_LE(liste[i].sonuc.rms_m, liste.back().sonuc.rms_m * 1.05) << liste[i].kaynak;
}

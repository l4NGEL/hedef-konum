// düz zemin ve elipsoid kesişimi
#include <gtest/gtest.h>

#include <cmath>

#include "hk/geolokasyon.hpp"

using namespace hk;

static Kamera kam() { return kamera_fovdan(1280, 720, derece2rad(60)); }

static Gozlem merkez_gozlem(double h, double yaw_deg, double tilt_deg) {
  Gozlem g;
  g.iha = lla_derece(40.0, 33.0, h);
  g.govde = euler_derece(0, 0, yaw_deg);
  g.tilt_rad = derece2rad(tilt_deg);
  const Kamera k = kam();
  g.u = k.cx;
  g.v = k.cy;
  return g;
}

TEST(Zemin4, NadirHedefTamAltta) {
  Gozlem g = merkez_gozlem(1500, 0, -90);
  Sonuc s = duz_zemin(kam(), g, 500);
  ASSERT_TRUE(s.bulundu);
  EXPECT_NEAR(s.menzil_m, 1000, 1e-6);
  EXPECT_NEAR(yatay_mesafe(g.iha, s.hedef), 0, 1e-6);
}

TEST(Zemin4, Kirkbes45DereceIleri) {
  // 1000 m yukarıdan 45° aşağı, kuzeye bakış: hedef ~1000 m kuzeyde
  Gozlem g = merkez_gozlem(1000, 0, -45);
  Sonuc s = duz_zemin(kam(), g, 0);
  ASSERT_TRUE(s.bulundu);
  Vek3 n = ecef_to_ned(lla_to_ecef(s.hedef), g.iha);
  EXPECT_NEAR(n.x, 1000, 1e-6);
  EXPECT_NEAR(n.y, 0, 1e-6);
  EXPECT_NEAR(s.menzil_m, 1000 * std::sqrt(2.0), 1e-6);
}

TEST(Zemin4, UfkaBakincaBulunmaz) {
  EXPECT_FALSE(duz_zemin(kam(), merkez_gozlem(1000, 0, 0), 0).bulundu);
  EXPECT_FALSE(duz_zemin(kam(), merkez_gozlem(1000, 0, 10), 0).bulundu);
}

TEST(Zemin4, ElipsoidYakindaDuzZeminleAyni) {
  Gozlem g = merkez_gozlem(1500, 30, -60);
  Sonuc a = duz_zemin(kam(), g, 500), b = elipsoid(kam(), g, 500);
  ASSERT_TRUE(a.bulundu && b.bulundu);
  EXPECT_LT(yatay_mesafe(a.hedef, b.hedef), 0.2);  // ~600 m menzilde fark ihmal edilir
}

TEST(Zemin4, ElipsoidSonucuIstenenYukseklikte) {
  Sonuc b = elipsoid(kam(), merkez_gozlem(3000, 0, -10), 200);
  ASSERT_TRUE(b.bulundu);
  EXPECT_NEAR(b.hedef.h_m, 200, 0.05);
}

TEST(Zemin4, DuzZeminUzaktaBozulur) {
  // Sığ bakış (tilt -5°), 2000 m yükseklik: hedef ~23 km ötede.
  // Düz zemin Dünya'nın eğriliğini bilmediği için hedefi "yakında" sanır.
  // Beklenti: d mesafesinde zemin teğet düzlemden d²/2R aşağıda; ışın o
  // farkı kapatmak için tanθ eğimiyle d²/(2R·tanθ) kadar daha ilerler.
  Gozlem g = merkez_gozlem(2000, 0, -5);
  Sonuc a = duz_zemin(kam(), g, 0), b = elipsoid(kam(), g, 0);
  ASSERT_TRUE(a.bulundu && b.bulundu);
  const double d = 2000.0 / std::tan(derece2rad(5));
  const double dusme = d * d / (2 * 6371000.0);
  const double beklenen = dusme / std::tan(derece2rad(5));
  const double fark = yatay_mesafe(a.hedef, b.hedef);
  EXPECT_NEAR(fark, beklenen, 0.1 * beklenen);  // ~470-490 m
  EXPECT_NEAR(a.hedef.h_m, dusme, 0.1 * dusme); // "zemin" dediği nokta ~41 m havada
}

TEST(Zemin4, IsinEcefBirim) {
  EXPECT_NEAR(norm(isin_ecef(kam(), merkez_gozlem(1000, 77, -33))), 1.0, 1e-12);
}

#include <gtest/gtest.h>

#include "hk/geodezi.hpp"

using namespace hk;

// --- LLA -> ECEF: elle bildiğimiz noktalar ---------------------------------

TEST(LlaEcef, EkvatorVeGreenwich) {
  // enlem 0, boylam 0, h 0  -> X ekseni üzerinde, ekvator yarıçapı kadar
  Vek3 e = lla_to_ecef(lla_derece(0, 0, 0));
  EXPECT_NEAR(e.x, wgs84::a, 1e-6);
  EXPECT_NEAR(e.y, 0.0, 1e-6);
  EXPECT_NEAR(e.z, 0.0, 1e-6);
}

TEST(LlaEcef, KuzeyKutbu) {
  // Kutupta Dünya'nın merkezine uzaklık 'b' (a'dan ~21 km kısa)
  Vek3 e = lla_to_ecef(lla_derece(90, 0, 0));
  EXPECT_NEAR(e.x, 0.0, 1e-6);
  EXPECT_NEAR(e.z, wgs84::b, 1e-6);
}

TEST(LlaEcef, Boylam90DerecedeYEkseni) {
  Vek3 e = lla_to_ecef(lla_derece(0, 90, 1000));
  EXPECT_NEAR(e.x, 0.0, 1e-6);
  EXPECT_NEAR(e.y, wgs84::a + 1000, 1e-6);
}

// --- ECEF -> LLA: gidiş-dönüş ----------------------------------------------

TEST(EcefLla, GidisDonus) {
  const LLA noktalar[] = {
      lla_derece(40.128, 32.995, 953),     // Esenboğa civarı
      lla_derece(-33.9, 151.2, 50),        // Sidney
      lla_derece(89.999, 10, 2000),        // kutba çok yakın
      lla_derece(0, -179.9, 10000),        // tarih çizgisi
  };
  for (const LLA& p : noktalar) {
    LLA q = ecef_to_lla(lla_to_ecef(p));
    EXPECT_NEAR(q.lat_rad, p.lat_rad, 1e-11);  // ~0,06 mm
    EXPECT_NEAR(q.lon_rad, p.lon_rad, 1e-11);
    EXPECT_NEAR(q.h_m, p.h_m, 1e-5);
  }
}

// --- NED ---------------------------------------------------------------------

TEST(Ned, ReferansNoktasiSifir) {
  LLA ref = lla_derece(40.128, 32.995, 953);
  Vek3 n = ecef_to_ned(lla_to_ecef(ref), ref);
  EXPECT_NEAR(norm(n), 0.0, 1e-6);
}

TEST(Ned, YukariCikmakDownuAzaltir) {
  // Aynı noktanın 50 m yukarısı: N=0, E=0, D=-50 (Aşağı eksen ters)
  LLA ref = lla_derece(40.128, 32.995, 953);
  Vek3 n = ecef_to_ned(lla_to_ecef(lla_derece(40.128, 32.995, 1003)), ref);
  EXPECT_NEAR(n.x, 0.0, 1e-6);
  EXPECT_NEAR(n.y, 0.0, 1e-6);
  EXPECT_NEAR(n.z, -50.0, 1e-6);
}

TEST(Ned, BirazKuzeyPozitifNorth) {
  LLA ref = lla_derece(40.128, 32.995, 953);
  // 0,001 derece enlem ~ 111 m kuzey
  Vek3 n = ecef_to_ned(lla_to_ecef(lla_derece(40.129, 32.995, 953)), ref);
  EXPECT_NEAR(n.x, 111.0, 1.0);
  EXPECT_NEAR(n.y, 0.0, 1e-3);
  EXPECT_GT(n.z, 0.0);  // Dünya eğri: 111 m ötede zemin biraz "aşağıda" (~1 mm)
}

TEST(Ned, MatrisDik) {
  // Dönme matrisi: R * R^T = I olmalı (eksenler birim ve birbirine dik)
  Mat3 R = ecef_to_ned_matrisi(lla_derece(40.128, 32.995, 953));
  Mat3 Rt = R.devrik();
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      double s = 0;
      for (int k = 0; k < 3; ++k) s += R.m[i][k] * Rt.m[k][j];
      EXPECT_NEAR(s, i == j ? 1.0 : 0.0, 1e-12);
    }
}

// --- NED -> ECEF ters dönüşüm ----------------------------------------------

TEST(Ned, NedEcefGidisDonus) {
  LLA ref = lla_derece(40.128, 32.995, 953);
  Vek3 ned{1200.0, -350.0, 80.0};
  Vek3 geri = ecef_to_ned(ned_to_ecef(ned, ref), ref);
  EXPECT_NEAR(geri.x, ned.x, 1e-6);
  EXPECT_NEAR(geri.y, ned.y, 1e-6);
  EXPECT_NEAR(geri.z, ned.z, 1e-6);
}

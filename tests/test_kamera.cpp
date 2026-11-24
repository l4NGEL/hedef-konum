#include <gtest/gtest.h>

#include <cmath>

#include "hk/donus.hpp"
#include "hk/kamera.hpp"

using namespace hk;

static Kamera ornek() {  // 1280x720, 60 derece HFOV
  return kamera_fovdan(1280, 720, derece2rad(60));
}

TEST(Kamera3, MerkezPikseliOptikEksen) {
  Kamera k = ornek();
  Vek3 r = piksel_to_isin(k, k.cx, k.cy);
  EXPECT_NEAR(r.x, 0, 1e-12);
  EXPECT_NEAR(r.y, 0, 1e-12);
  EXPECT_NEAR(r.z, 1, 1e-12);
}

TEST(Kamera3, KenarPikseliYariFov) {
  // Görüntünün sağ kenarı (piksel kenarı, w - 0.5) optik eksenle HFOV/2 yapar
  Kamera k = ornek();
  Vek3 r = piksel_to_isin(k, k.genislik - 0.5, k.cy);
  double aci = std::atan2(r.x, r.z);
  EXPECT_NEAR(rad2derece(aci), 30.0, 1e-9);
}

TEST(Kamera3, IsinBirimUzunlukta) {
  Kamera k = ornek();
  EXPECT_NEAR(norm(piksel_to_isin(k, 13, 700)), 1.0, 1e-12);
}

TEST(Kamera3, IzdusumGidisDonus) {
  Kamera k = ornek();
  for (double u : {0.0, 200.5, 640.0, 1279.0})
    for (double v : {0.0, 333.3, 719.0}) {
      double u2, v2;
      Vek3 r = piksel_to_isin(k, u, v);
      ASSERT_TRUE(izdusur(k, 57.0 * r, u2, v2));  // ışın üzerinde herhangi nokta
      EXPECT_NEAR(u2, u, 1e-9);
      EXPECT_NEAR(v2, v, 1e-9);
    }
}

TEST(Kamera3, BozulmaGidisDonus) {
  Kamera k = ornek();
  k.k1 = -0.28;  // belirgin fıçı bozulması
  k.k2 = 0.08;
  for (double u : {0.0, 320.0, 1279.0})
    for (double v : {0.0, 719.0}) {
      double u2, v2;
      ASSERT_TRUE(izdusur(k, piksel_to_isin(k, u, v), u2, v2));
      EXPECT_NEAR(u2, u, 1e-6);
      EXPECT_NEAR(v2, v, 1e-6);
    }
}

TEST(Kamera3, BozulmayiYokSaymakKosedeAciHatasi) {
  // Bozulmalı lensin köşe pikseli: bozulmayı bilmezsek ışın ne kadar sapar?
  Kamera gercek = ornek();
  gercek.k1 = -0.28;
  gercek.k2 = 0.08;
  Kamera saf = ornek();
  Vek3 a = piksel_to_isin(gercek, 0, 0), b = piksel_to_isin(saf, 0, 0);
  double hata_derece = rad2derece(std::acos(nokta(a, b)));
  EXPECT_GT(hata_derece, 2.0);  // 1 km uzakta ~35 m'den fazla
}

TEST(Kamera3, ArkadakiNoktaIzdusmez) {
  double u, v;
  EXPECT_FALSE(izdusur(ornek(), {0, 0, -5}, u, v));
}

TEST(Kamera3, NadirKameradaGoruntuYonleri) {
  // Kuzeye düz uçuş, gimbal dümdüz aşağı: görüntünün üstü kuzey, sağı doğu.
  Kamera k = ornek();
  Mat3 R = R_ned_kam(euler_derece(0, 0, 0), 0, derece2rad(-90));
  Vek3 ust = R * piksel_to_isin(k, k.cx, 0);
  Vek3 sag = R * piksel_to_isin(k, k.genislik - 1, k.cy);
  EXPECT_GT(ust.x, 0.1);   // kuzey bileşeni pozitif
  EXPECT_GT(sag.y, 0.1);   // doğu bileşeni pozitif
  EXPECT_GT(ust.z, 0.0);   // ikisi de hâlâ aşağı bakıyor
  EXPECT_GT(sag.z, 0.0);
}

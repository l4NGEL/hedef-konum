#include <gtest/gtest.h>

#include <cmath>

#include "hk/donus.hpp"

using namespace hk;

static void yakin(Vek3 a, Vek3 b, double tol = 1e-12) {
  EXPECT_NEAR(a.x, b.x, tol);
  EXPECT_NEAR(a.y, b.y, tol);
  EXPECT_NEAR(a.z, b.z, tol);
}

const Vek3 ILERI{1, 0, 0}, SAG{0, 1, 0}, ASAGI{0, 0, 1};

// --- Gövde açıları: her biri tek başına sezgiye uymalı ----------------------

TEST(Govde, Yaw90DoguyaBakar) {
  // Burun doğuya dönünce gövde "ileri" = NED'de Doğu
  Mat3 R = R_ned_govde(euler_derece(0, 0, 90));
  yakin(R * ILERI, {0, 1, 0});
}

TEST(Govde, PitchPozitifBurunYukari) {
  Mat3 R = R_ned_govde(euler_derece(0, 10, 0));
  Vek3 v = R * ILERI;
  EXPECT_GT(v.x, 0.98);
  EXPECT_LT(v.z, 0.0);  // D negatif = yukarı
  EXPECT_NEAR(v.z, -std::sin(derece2rad(10)), 1e-12);
}

TEST(Govde, Roll90SagKanatAsagi) {
  Mat3 R = R_ned_govde(euler_derece(90, 0, 0));
  yakin(R * SAG, {0, 0, 1});
}

TEST(Govde, SiraOnemli) {
  // Önce yaw sonra pitch != önce pitch sonra yaw.
  Mat3 a = Rz(derece2rad(90)) * Ry(derece2rad(30));
  Mat3 b = Ry(derece2rad(30)) * Rz(derece2rad(90));
  Vek3 va = a * ILERI, vb = b * ILERI;
  EXPECT_GT(norm(va - vb), 0.5);  // aynı açılar, çok farklı yön
}

TEST(Govde, MatristenEulerGeriDonus) {
  Euler e = euler_derece(12.5, -7.0, 243.0 - 360.0);
  Euler g = matris_to_euler(R_ned_govde(e));
  EXPECT_NEAR(g.roll_rad, e.roll_rad, 1e-12);
  EXPECT_NEAR(g.pitch_rad, e.pitch_rad, 1e-12);
  EXPECT_NEAR(g.yaw_rad, e.yaw_rad, 1e-12);
}

// --- Kamera zinciri ----------------------------------------------------------

TEST(Kamera, OptikEksenGimbalIleri) {
  // Kameranın baktığı yön (kamera z) gimbal x olmalı
  yakin(R_gimbal_kam() * Vek3{0, 0, 1}, ILERI);
  yakin(R_gimbal_kam() * Vek3{1, 0, 0}, SAG);    // görüntüde sağ
  yakin(R_gimbal_kam() * Vek3{0, 1, 0}, ASAGI);  // görüntüde aşağı
}

TEST(Kamera, TiltEksi90DumduzAsagi) {
  // Düz uçuş, gimbal -90: kamera tam nadire (aşağı) bakar
  Mat3 R = R_ned_kam(euler_derece(0, 0, 0), 0, derece2rad(-90));
  yakin(R * Vek3{0, 0, 1}, {0, 0, 1});
}

TEST(Kamera, DoguyaUcarkenSagaBakmakGuney) {
  // Uçak doğuya, gimbal pan +90 (sağa), tilt 0: kamera güneye bakar
  Mat3 R = R_ned_kam(euler_derece(0, 0, 90), derece2rad(90), 0);
  yakin(R * Vek3{0, 0, 1}, {-1, 0, 0});
}

TEST(Kamera, ZincirHepDonmeMatrisi) {
  // Kaç dönüş çarparsak çarpalım sonuç R R^T = I kalmalı
  Mat3 R = R_ned_kam(euler_derece(5, -3, 120), derece2rad(40), derece2rad(-35));
  Mat3 I = R * R.devrik();
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) EXPECT_NEAR(I.m[i][j], i == j ? 1.0 : 0.0, 1e-12);
}

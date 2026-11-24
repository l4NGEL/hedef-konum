#pragma once
// Dönüşler. İHA gövdesi, gimbal ve kamera eksenleri.
//
// İSİM KURALI (dönüş hatalarının çoğunu bu önler):
//   R_a_b : "b" çerçevesinde yazılmış bir vektörü "a" çerçevesine çevirir.
//           v_a = R_a_b * v_b
//   Zincirleme yaparken içteki harfler birbirini götürür:
//           R_ned_kam = R_ned_govde * R_govde_gimbal * R_gimbal_kam
//                            ^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^
//
// Çerçeveler:
//   NED    : Kuzey, Doğu, Aşağı (geodezi.hpp)
//   GÖVDE  : FRD -> x burun yönü (ileri), y sağ kanat, z aşağı
//   GIMBAL : x optik eksen (kameranın baktığı yön), y sağ, z aşağı
//   KAMERA : OpenCV optik çerçevesi -> z ileri (optik eksen), x sağ, y aşağı
#include "hk/vek.hpp"

namespace hk {

// Tek eksen etrafında dönüşler (sağ el kuralı, pozitif açı)
Mat3 Rx(double aci_rad);
Mat3 Ry(double aci_rad);
Mat3 Rz(double aci_rad);

struct Euler {
  double roll_rad{0};   // yatış: sağ kanat aşağı  = pozitif
  double pitch_rad{0};  // yunuslama: burun yukarı = pozitif
  double yaw_rad{0};    // sapma: kuzeyden doğuya  = pozitif (pusula yönü)
};

Euler euler_derece(double roll, double pitch, double yaw);

// Havacılık standardı ZYX sırası: önce yaw, sonra pitch, sonra roll.
// R_ned_govde = Rz(yaw) * Ry(pitch) * Rx(roll)
Mat3 R_ned_govde(const Euler& e);

// Gimbal iki eksenli: önce pan (gövde z etrafında), sonra tilt (y etrafında).
// tilt = -90 derece -> dümdüz aşağı bakar.
Mat3 R_govde_gimbal(double pan_rad, double tilt_rad);

// Sabit eksen yeniden adlandırma: kamera z -> gimbal x, kamera x -> gimbal y,
// kamera y -> gimbal z.
Mat3 R_gimbal_kam();

// Hepsi birden: kamera çerçevesindeki bir vektörü NED'e çevirir.
Mat3 R_ned_kam(const Euler& govde, double pan_rad, double tilt_rad);

// Matristen Euler açılarını geri çıkarır (ZYX). pitch = ±90'da tekil.
Euler matris_to_euler(const Mat3& R_ned_govde);

}  // namespace hk

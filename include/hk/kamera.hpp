#pragma once
// Pinhole kamera modeli. Piksel <-> kamera çerçevesinde ışın.
//
//   Kamera çerçevesi (OpenCV): z ileri (optik eksen), x sağ, y aşağı.
//   Görüntü: (u, v) piksel, (0,0) sol üst köşe, u sağa, v aşağı artar.
//
//   İzdüşüm (3B -> piksel):
//     x' = X/Z, y' = Y/Z               normalize düzlem (Z = 1 düzlemi)
//     (x'', y'') = bozulma(x', y')     mercek eğriltir
//     u = fx * x'' + cx,  v = fy * y'' + cy
//
//   Geolocation'da tersini yaparız: piksel -> ışın yönü. Uzaklığı (Z)
//   kamera tek başına bilemez; onu zemin/DEM kesişimi buluyor (geolokasyon, dem).
#include "hk/vek.hpp"

namespace hk {

struct Kamera {
  int genislik{0}, yukseklik{0};  // piksel
  double fx{0}, fy{0};            // odak uzaklığı [piksel]
  double cx{0}, cy{0};            // optik merkez [piksel]
  double k1{0}, k2{0};            // radyal bozulma (Brown-Conrady, ilk iki terim)
};

// Yatay görüş açısından (HFOV) ideal kamera: kare piksel, merkez ortada.
Kamera kamera_fovdan(int genislik, int yukseklik, double hfov_rad);

// Normalize düzlemde radyal bozulma ve tersi.
void boz(const Kamera& k, double x, double y, double& xb, double& yb);
void boz_geri(const Kamera& k, double xb, double yb, double& x, double& y);

// Kamera çerçevesindeki bir noktayı/yönü piksele çevirir (Z > 0 olmalı).
bool izdusur(const Kamera& k, const Vek3& p_kam, double& u, double& v);

// Pikselden kamera çerçevesinde BİRİM ışın.
Vek3 piksel_to_isin(const Kamera& k, double u, double v);

}  // namespace hk

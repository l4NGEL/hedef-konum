#pragma once
// Işını zeminle kesiştirmek.
//
//   piksel --(kamera)--> kamera ışını --(donus)--> NED ışını --(geodezi)--> ECEF ışını
//   ECEF ışını + zemin modeli  ==>  hedefin konumu
//
// İki zemin modeli var:
//   1) Düz zemin   : İHA'nın altındaki teğet düzlem, sabit yükseklik. Basit ama
//                    Dünya'nın eğriliğini yok sayar; uzak hedefte bozulur.
//   2) Elipsoid    : WGS84 elipsoidini h kadar şişirip ışını onunla kesiştirir.
//                    Eğriliği hesaba katar ama araziyi (tepe/vadi) bilmez.
// Arazinin kendisi dem.hpp'de.
#include "hk/donus.hpp"
#include "hk/geodezi.hpp"
#include "hk/kamera.hpp"

namespace hk {

// Tek bir kare için elimizdeki her şey.
struct Gozlem {
  LLA iha;            // İHA konumu (GPS)
  Euler govde;        // İHA yönelimi (IMU / AHRS)
  double pan_rad{0};  // gimbal açıları (gövdeye göre)
  double tilt_rad{0};
  double u{0}, v{0};  // hedefin piksel konumu (tespitçinin kutu merkezi)
};

struct Sonuc {
  bool bulundu{false};
  LLA hedef;           // bulunan nokta
  double menzil_m{0};  // kameradan hedefe eğik mesafe (slant range)
  int adim{0};         // yinelemeli/yürütmeli yöntemlerde harcanan adım
};

// Işının yönü (birim vektör)
Vek3 isin_ned(const Kamera& k, const Gozlem& g);
Vek3 isin_ecef(const Kamera& k, const Gozlem& g);

// 1) Düz zemin: İHA altındaki teğet düzlem, zemin_h yüksekliğinde.
Sonuc duz_zemin(const Kamera& k, const Gozlem& g, double zemin_h_m);

// 2) Işın ile (a+h, b+h) elipsoidinin kesişimi. o, d: ECEF başlangıç ve birim yön.
Sonuc elipsoid_kesisim(const Vek3& o, const Vek3& d, double h_m);
Sonuc elipsoid(const Kamera& k, const Gozlem& g, double zemin_h_m);

}  // namespace hk

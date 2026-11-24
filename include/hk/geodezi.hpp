#pragma once
// Dünya üzerindeki koordinat sistemleri.
//
//   LLA  : enlem (lat), boylam (lon), elipsoid yüksekliği (h)  -> GPS'in verdiği
//   ECEF : Dünya merkezli, Dünya'ya sabit kartezyen (metre)    -> hesap yapılan yer
//   NED  : yerel Kuzey-Doğu-Aşağı (metre), bir referans noktada -> uçağın "etrafı"
#include "hk/vek.hpp"

namespace hk {

// WGS84 elipsoidi (GPS'in kullandığı Dünya modeli)
namespace wgs84 {
constexpr double a = 6378137.0;                // ekvator yarıçapı [m]
constexpr double f = 1.0 / 298.257223563;      // basıklık
constexpr double b = a * (1.0 - f);            // kutup yarıçapı [m]
constexpr double e2 = f * (2.0 - f);           // birinci dışmerkezlik karesi
}  // namespace wgs84

struct LLA {
  double lat_rad{0};  // enlem  [radyan]
  double lon_rad{0};  // boylam [radyan]
  double h_m{0};      // elipsoid üstü yükseklik [m]  (deniz seviyesi DEĞİL)
};

LLA lla_derece(double lat_deg, double lon_deg, double h_m);

Vek3 lla_to_ecef(const LLA& p);
LLA ecef_to_lla(const Vek3& ecef);

// ECEF farkını referans noktasının NED eksenlerine çeviren dönüş matrisi.
Mat3 ecef_to_ned_matrisi(const LLA& ref);

Vek3 ecef_to_ned(const Vek3& ecef, const LLA& ref);
Vek3 ned_to_ecef(const Vek3& ned, const LLA& ref);

// ECEF'te bir YÖN vektörünü (konum değil) NED'e / NED'den çevirir: sadece döndürme.
inline Vek3 yon_ecef_to_ned(const Vek3& d, const LLA& ref) { return ecef_to_ned_matrisi(ref) * d; }
inline Vek3 yon_ned_to_ecef(const Vek3& d, const LLA& ref) {
  return ecef_to_ned_matrisi(ref).devrik() * d;
}

// İki nokta arasındaki yatay (kuzey-doğu) mesafe, 'a' noktasının NED düzleminde.
double yatay_mesafe(const LLA& a, const LLA& b);

}  // namespace hk

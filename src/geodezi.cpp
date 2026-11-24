#include "hk/geodezi.hpp"

#include <cmath>

namespace hk {

LLA lla_derece(double lat_deg, double lon_deg, double h_m) {
  return {derece2rad(lat_deg), derece2rad(lon_deg), h_m};
}

// Dik doğrultu eğrilik yarıçapı N(φ): o enlemde elipsoide dik çizginin
// dönme eksenine kadar olan uzunluğu. Küre olsaydı hep 'a' olurdu.
static double N_yaricap(double lat) {
  const double s = std::sin(lat);
  return wgs84::a / std::sqrt(1.0 - wgs84::e2 * s * s);
}

Vek3 lla_to_ecef(const LLA& p) {
  const double N = N_yaricap(p.lat_rad);
  const double cl = std::cos(p.lat_rad), sl = std::sin(p.lat_rad);
  const double co = std::cos(p.lon_rad), so = std::sin(p.lon_rad);
  return {(N + p.h_m) * cl * co,
          (N + p.h_m) * cl * so,
          (N * (1.0 - wgs84::e2) + p.h_m) * sl};  // Z'de (1-e²): elipsoid basık
}

// Tersi kapalı formda kolay değil; enlemi yineleyerek buluyoruz.
LLA ecef_to_lla(const Vek3& e) {
  const double lon = std::atan2(e.y, e.x);
  const double p = std::hypot(e.x, e.y);  // dönme ekseninden uzaklık

  double lat = std::atan2(e.z, p * (1.0 - wgs84::e2));  // ilk tahmin
  double h = 0.0;
  for (int i = 0; i < 10; ++i) {
    const double N = N_yaricap(lat);
    const double sl = std::sin(lat), cl = std::cos(lat);
    // Kutuplarda da kararlı yükseklik formülü (p/cos(lat) kutupta patlar)
    h = p * cl + e.z * sl - wgs84::a * std::sqrt(1.0 - wgs84::e2 * sl * sl);
    const double yeni = std::atan2(e.z, p * (1.0 - wgs84::e2 * N / (N + h)));
    const bool bitti = std::abs(yeni - lat) < 1e-13;
    lat = yeni;
    if (bitti) break;
  }
  return {lat, lon, h};
}

// Satırlar: referans noktadaki Kuzey, Doğu, Aşağı birim vektörlerinin
// ECEF'teki ifadesi. Bir vektörü bu satırlarla nokta çarpmak, onun
// kuzey/doğu/aşağı bileşenlerini verir.
Mat3 ecef_to_ned_matrisi(const LLA& ref) {
  const double sl = std::sin(ref.lat_rad), cl = std::cos(ref.lat_rad);
  const double so = std::sin(ref.lon_rad), co = std::cos(ref.lon_rad);
  Mat3 R;
  R.m[0][0] = -sl * co; R.m[0][1] = -sl * so; R.m[0][2] = cl;    // Kuzey
  R.m[1][0] = -so;      R.m[1][1] = co;       R.m[1][2] = 0.0;   // Doğu
  R.m[2][0] = -cl * co; R.m[2][1] = -cl * so; R.m[2][2] = -sl;   // Aşağı
  return R;
}

Vek3 ecef_to_ned(const Vek3& ecef, const LLA& ref) {
  const Vek3 fark = ecef - lla_to_ecef(ref);  // önce referansa göre öteleme
  return ecef_to_ned_matrisi(ref) * fark;     // sonra döndürme
}

Vek3 ned_to_ecef(const Vek3& ned, const LLA& ref) {
  // ecef_to_ned'in tersi: önce geri döndür (R^-1 = R^T), sonra geri ötele.
  return lla_to_ecef(ref) + ecef_to_ned_matrisi(ref).devrik() * ned;
}

double yatay_mesafe(const LLA& a, const LLA& b) {
  const Vek3 n = ecef_to_ned(lla_to_ecef(b), a);
  return std::hypot(n.x, n.y);
}

}  // namespace hk

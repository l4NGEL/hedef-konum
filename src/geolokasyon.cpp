#include "hk/geolokasyon.hpp"

#include <cmath>
#include <utility>

namespace hk {

Vek3 isin_ned(const Kamera& k, const Gozlem& g) {
  return R_ned_kam(g.govde, g.pan_rad, g.tilt_rad) * piksel_to_isin(k, g.u, g.v);
}

Vek3 isin_ecef(const Kamera& k, const Gozlem& g) {
  return yon_ned_to_ecef(isin_ned(k, g), g.iha);
}

Sonuc duz_zemin(const Kamera& k, const Gozlem& g, double zemin_h) {
  // NED'de düzlem: D = (İHA yüksekliği - zemin yüksekliği) = sabit.
  // Işın: p(t) = t * d. D bileşeni eşitlenince t = D0 / d.z
  Sonuc s;
  const Vek3 d = isin_ned(k, g);
  const double D0 = g.iha.h_m - zemin_h;
  if (D0 <= 0.0 || d.z <= 1e-9) return s;  // zeminin altındayız ya da ufka/göğe bakıyoruz
  const double t = D0 / d.z;
  s.bulundu = true;
  s.menzil_m = t;
  s.hedef = ecef_to_lla(ned_to_ecef(t * d, g.iha));
  return s;
}

Sonuc elipsoid_kesisim(const Vek3& o, const Vek3& d, double h) {
  // (x² + y²)/A² + z²/B² = 1,  A = a+h, B = b+h,  p = o + t d
  // t cinsinden ikinci derece denklem: qa t² + qb t + qc = 0
  // Not: (a+h, b+h) elipsoidi "her yerde tam h yükseklik" yüzeyi değildir, ama
  // birkaç km yükseklikte fark milimetre düzeyinde; geolocation için yeterli.
  Sonuc s;
  const double A2 = (wgs84::a + h) * (wgs84::a + h);
  const double B2 = (wgs84::b + h) * (wgs84::b + h);
  const double qa = (d.x * d.x + d.y * d.y) / A2 + d.z * d.z / B2;
  const double qb = 2.0 * ((o.x * d.x + o.y * d.y) / A2 + o.z * d.z / B2);
  const double qc = (o.x * o.x + o.y * o.y) / A2 + o.z * o.z / B2 - 1.0;
  if (qc <= 0.0) return s;  // başlangıç noktası yüzeyin içinde/altında
  const double disk = qb * qb - 4.0 * qa * qc;
  if (disk < 0.0) return s;  // ışın yüzeyi ıskalıyor (ufkun üstüne bakıyor)
  // Sayısal olarak kararlı kök: büyük sayılardan küçük fark çıkarmamak için
  const double q = -0.5 * (qb + std::copysign(std::sqrt(disk), qb));
  double t1 = q / qa, t2 = qc / q;
  if (t1 > t2) std::swap(t1, t2);
  const double t = (t1 > 0.0) ? t1 : t2;  // ilk (yakın) çarpma noktası
  if (t <= 0.0) return s;                 // yüzey arkamızda
  s.bulundu = true;
  s.menzil_m = t;
  s.hedef = ecef_to_lla(o + t * d);
  return s;
}

Sonuc elipsoid(const Kamera& k, const Gozlem& g, double zemin_h) {
  return elipsoid_kesisim(lla_to_ecef(g.iha), isin_ecef(k, g), zemin_h);
}

}  // namespace hk

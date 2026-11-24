#pragma once
// Uygulamalar ve testlerin ortak kullandığı sentetik tepelik arazi.
// Gerçek bir yerin yüksekliği DEĞİL; sadece eğimli, tepeli bir zemin.
#include <cmath>

#include "hk/dem.hpp"

namespace hk {

constexpr double ARAZI_LAT0 = 39.95, ARAZI_LON0 = 32.70;

inline FonksiyonDem sentetik_arazi() {
  return FonksiyonDem([](double lat, double lon) {
    const double dn = (lat - ARAZI_LAT0) * 111000.0;
    const double de = (lon - ARAZI_LON0) * 111000.0 * std::cos(derece2rad(ARAZI_LAT0));
    return 1000.0 + 120.0 * std::sin(dn / 900.0) * std::cos(de / 1300.0) +
           60.0 * std::sin((dn + de) / 400.0);
  });
}

}  // namespace hk

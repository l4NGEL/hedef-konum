#include "hk/donus.hpp"

#include <cmath>

namespace hk {

// Her biri: o eksen sabit kalır, diğer ikisi düzlemde döner.
Mat3 Rx(double a) {
  const double c = std::cos(a), s = std::sin(a);
  Mat3 R;
  R.m[0][0] = 1;
  R.m[1][1] = c; R.m[1][2] = -s;
  R.m[2][1] = s; R.m[2][2] = c;
  return R;
}

Mat3 Ry(double a) {
  const double c = std::cos(a), s = std::sin(a);
  Mat3 R;
  R.m[0][0] = c;  R.m[0][2] = s;   // dikkat: işaretler Rx/Rz'ye göre
  R.m[1][1] = 1;                   // "ters" görünür, çünkü z×x = y
  R.m[2][0] = -s; R.m[2][2] = c;
  return R;
}

Mat3 Rz(double a) {
  const double c = std::cos(a), s = std::sin(a);
  Mat3 R;
  R.m[0][0] = c; R.m[0][1] = -s;
  R.m[1][0] = s; R.m[1][1] = c;
  R.m[2][2] = 1;
  return R;
}

Euler euler_derece(double roll, double pitch, double yaw) {
  return {derece2rad(roll), derece2rad(pitch), derece2rad(yaw)};
}

Mat3 R_ned_govde(const Euler& e) {
  // Sağdan sola oku: vektör önce roll, sonra pitch, sonra yaw ile döner.
  // Ama çerçeve gözüyle: NED'den başla, yaw kadar dön, yeni eksende pitch,
  // en son roll. Aynı şeyin iki okunuşu.
  return Rz(e.yaw_rad) * Ry(e.pitch_rad) * Rx(e.roll_rad);
}

Mat3 R_govde_gimbal(double pan, double tilt) {
  // Gimbal, gövdeye takılı mini bir "yaw + pitch" sistemi.
  return Rz(pan) * Ry(tilt);
}

Mat3 R_gimbal_kam() {
  // Sütunlar: kamera eksenlerinin gimbal çerçevesindeki karşılığı.
  //   kamera x (sağ)   -> gimbal y = (0,1,0)
  //   kamera y (aşağı) -> gimbal z = (0,0,1)
  //   kamera z (ileri) -> gimbal x = (1,0,0)
  Mat3 R;
  R.m[0][2] = 1;
  R.m[1][0] = 1;
  R.m[2][1] = 1;
  return R;
}

Mat3 R_ned_kam(const Euler& govde, double pan, double tilt) {
  return R_ned_govde(govde) * R_govde_gimbal(pan, tilt) * R_gimbal_kam();
}

Euler matris_to_euler(const Mat3& R) {
  // R = Rz(y) Ry(p) Rx(r) açılınca:
  //   R[2][0] = -sin(p)
  //   R[1][0]/R[0][0] = tan(y)
  //   R[2][1]/R[2][2] = tan(r)
  Euler e;
  e.pitch_rad = -std::asin(std::fmax(-1.0, std::fmin(1.0, R.m[2][0])));
  e.yaw_rad = std::atan2(R.m[1][0], R.m[0][0]);
  e.roll_rad = std::atan2(R.m[2][1], R.m[2][2]);
  return e;
}

}  // namespace hk

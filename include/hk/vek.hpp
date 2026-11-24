#pragma once
// Küçük 3B vektör ve 3x3 matris. Eigen kullanmıyoruz; dönüşümlerin
// içini görmek için bilerek kendimiz yazıyoruz.
#include <cmath>

namespace hk {

struct Vek3 {
  double x{0}, y{0}, z{0};
};

inline Vek3 operator+(Vek3 a, Vek3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vek3 operator-(Vek3 a, Vek3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vek3 operator*(double s, Vek3 v) { return {s * v.x, s * v.y, s * v.z}; }
inline double nokta(Vek3 a, Vek3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline double norm(Vek3 v) { return std::sqrt(nokta(v, v)); }

// Satır düzenli 3x3 matris: m[satır][sütun]
struct Mat3 {
  double m[3][3]{};

  Vek3 operator*(Vek3 v) const {
    return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
  }

  Mat3 operator*(const Mat3& o) const {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 3; ++k) r.m[i][j] += m[i][k] * o.m[k][j];
    return r;
  }

  static Mat3 birim() {
    Mat3 I;
    I.m[0][0] = I.m[1][1] = I.m[2][2] = 1.0;
    return I;
  }

  Mat3 devrik() const {  // transpose
    Mat3 t;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) t.m[i][j] = m[j][i];
    return t;
  }

  Mat3 operator+(const Mat3& o) const {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) r.m[i][j] = m[i][j] + o.m[i][j];
    return r;
  }

  double det() const {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  }

  // Genel tersi (dönme matrisi DEĞİLSE). Ek matris / determinant.
  // Dönme matrisi için bunu kullanma, devrik() hem hızlı hem tam doğru.
  Mat3 ters() const {
    const double d = det();
    Mat3 r;
    r.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) / d;
    r.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) / d;
    r.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) / d;
    r.m[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) / d;
    r.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) / d;
    r.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) / d;
    r.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) / d;
    r.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) / d;
    r.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) / d;
    return r;
  }
};

inline Vek3 birimle(Vek3 v) { return (1.0 / norm(v)) * v; }

// I - d d^T : bir vektörün d'ye dik bileşenini veren izdüşüm matrisi
inline Mat3 dik_izdusum(Vek3 d) {
  const double a[3] = {d.x, d.y, d.z};
  Mat3 P = Mat3::birim();
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) P.m[i][j] -= a[i] * a[j];
  return P;
}

inline double derece2rad(double d) { return d * M_PI / 180.0; }
inline double rad2derece(double r) { return r * 180.0 / M_PI; }

}  // namespace hk

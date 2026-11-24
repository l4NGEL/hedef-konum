#include "hk/kamera.hpp"

#include <cmath>

namespace hk {

Kamera kamera_fovdan(int w, int h, double hfov) {
  // Görüntünün yarı genişliği (w/2) optik eksenden hfov/2 açıyla görünür:
  //   tan(hfov/2) = (w/2) / fx   ->   fx = (w/2) / tan(hfov/2)
  Kamera k;
  k.genislik = w;
  k.yukseklik = h;
  k.fx = k.fy = (w / 2.0) / std::tan(hfov / 2.0);
  // Piksel merkezleri tam sayıda: 0..w-1 aralığının ortası (w-1)/2
  k.cx = (w - 1) / 2.0;
  k.cy = (h - 1) / 2.0;
  return k;
}

void boz(const Kamera& k, double x, double y, double& xb, double& yb) {
  // Merkezden uzaklaştıkça büyüme/küçülme: r² ile ölçeklenen çarpan.
  // k1 < 0 -> fıçı (geniş açı lenslerde tipik), k1 > 0 -> yastık.
  const double r2 = x * x + y * y;
  const double olcek = 1.0 + k.k1 * r2 + k.k2 * r2 * r2;
  xb = x * olcek;
  yb = y * olcek;
}

void boz_geri(const Kamera& k, double xb, double yb, double& x, double& y) {
  // Kapalı ters formül yok. Sabit nokta yinelemesi:
  //   x = xb / olcek(x)  ->  tahmini her adımda düzelt.
  x = xb;
  y = yb;
  for (int i = 0; i < 20; ++i) {
    const double r2 = x * x + y * y;
    const double olcek = 1.0 + k.k1 * r2 + k.k2 * r2 * r2;
    const double nx = xb / olcek, ny = yb / olcek;
    const bool bitti = std::abs(nx - x) < 1e-14 && std::abs(ny - y) < 1e-14;
    x = nx;
    y = ny;
    if (bitti) break;
  }
}

bool izdusur(const Kamera& k, const Vek3& p, double& u, double& v) {
  if (p.z <= 0.0) return false;  // kameranın arkasında: görüntüye düşmez
  double xb, yb;
  boz(k, p.x / p.z, p.y / p.z, xb, yb);
  u = k.fx * xb + k.cx;
  v = k.fy * yb + k.cy;
  return true;
}

Vek3 piksel_to_isin(const Kamera& k, double u, double v) {
  // Adım adım izdüşümün tersi: pikselden normalize düzleme, bozulmayı geri
  // al, Z = 1 koyup birim vektöre çevir.
  double x, y;
  boz_geri(k, (u - k.cx) / k.fx, (v - k.cy) / k.fy, x, y);
  Vek3 r{x, y, 1.0};
  return (1.0 / norm(r)) * r;
}

}  // namespace hk

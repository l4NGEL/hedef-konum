// Tek gözlemden hedef konumu + belirsizlik. iha-takip gibi bir tespitçinin
// çıktısını bağlamak için komut satırı arayüzü. Çıktı tek satır JSON.
//
//   hk_coz --iha LAT LON H --govde ROLL PITCH YAW --gimbal PAN TILT
//          --piksel U V --kamera W H HFOV [--k1 K1 --k2 K2]
//          [--dem DOSYA.hgt | --zemin H] [--mc N]
// Açılar derece. --dem verilmezse --zemin yüksekliğinde (varsayılan 0) elipsoid.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include "hk/belirsizlik.hpp"

using namespace hk;

static void kullanim() {
  std::fprintf(stderr,
               "kullanim: hk_coz --iha LAT LON H --govde ROLL PITCH YAW --gimbal PAN TILT\n"
               "              --piksel U V --kamera W H HFOV [--k1 K1 --k2 K2]\n"
               "              [--dem DOSYA.hgt | --zemin H] [--mc N]\n");
}

int main(int argc, char** argv) {
  double lat = 0, lon = 0, h = 0, roll = 0, pitch = 0, yaw = 0, pan = 0, tilt = 0, u = -1, v = -1;
  int w = 0, hh = 0;
  double hfov = 0, k1 = 0, k2 = 0, zemin = 0;
  int mc = 0;
  std::string dem_yolu;
  bool iha = false, gov = false, gim = false, pik = false, kam_ok = false;

  auto al = [&](int& i, int n) {
    if (i + n >= argc) { kullanim(); std::exit(2); }
    return i;
  };
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (!std::strcmp(a, "--iha")) { al(i, 3); lat = atof(argv[++i]); lon = atof(argv[++i]); h = atof(argv[++i]); iha = true; }
    else if (!std::strcmp(a, "--govde")) { al(i, 3); roll = atof(argv[++i]); pitch = atof(argv[++i]); yaw = atof(argv[++i]); gov = true; }
    else if (!std::strcmp(a, "--gimbal")) { al(i, 2); pan = atof(argv[++i]); tilt = atof(argv[++i]); gim = true; }
    else if (!std::strcmp(a, "--piksel")) { al(i, 2); u = atof(argv[++i]); v = atof(argv[++i]); pik = true; }
    else if (!std::strcmp(a, "--kamera")) { al(i, 3); w = atoi(argv[++i]); hh = atoi(argv[++i]); hfov = atof(argv[++i]); kam_ok = true; }
    else if (!std::strcmp(a, "--k1")) { al(i, 1); k1 = atof(argv[++i]); }
    else if (!std::strcmp(a, "--k2")) { al(i, 1); k2 = atof(argv[++i]); }
    else if (!std::strcmp(a, "--dem")) { al(i, 1); dem_yolu = argv[++i]; }
    else if (!std::strcmp(a, "--zemin")) { al(i, 1); zemin = atof(argv[++i]); }
    else if (!std::strcmp(a, "--mc")) { al(i, 1); mc = atoi(argv[++i]); }
    else { kullanim(); return 2; }
  }
  if (!(iha && gov && gim && pik && kam_ok)) { kullanim(); return 2; }

  Kamera k = kamera_fovdan(w, hh, derece2rad(hfov));
  k.k1 = k1;
  k.k2 = k2;
  Gozlem g;
  g.iha = lla_derece(lat, lon, h);
  g.govde = euler_derece(roll, pitch, yaw);
  g.pan_rad = derece2rad(pan);
  g.tilt_rad = derece2rad(tilt);
  g.u = u;
  g.v = v;

  std::unique_ptr<Dem> dem;
  try {
    if (!dem_yolu.empty()) dem = std::make_unique<IzgaraDem>(IzgaraDem::hgt_oku(dem_yolu));
    else dem = std::make_unique<SabitDem>(zemin);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "DEM okunamadi: %s\n", e.what());
    return 1;
  }

  const Sonuc s = geolokasyon(k, g, *dem);
  if (!s.bulundu) {
    std::printf("{\"bulundu\":false}\n");
    return 3;
  }
  std::printf("{\"bulundu\":true,\"lat\":%.8f,\"lon\":%.8f,\"h\":%.2f,\"menzil_m\":%.1f",
              rad2derece(s.hedef.lat_rad), rad2derece(s.hedef.lon_rad), s.hedef.h_m, s.menzil_m);
  if (mc > 0) {
    const McSonuc m = monte_carlo(k, g, *dem, ornek_belirsizlik(), mc, 1);
    std::printf(",\"cep50_m\":%.1f,\"cep90_m\":%.1f", m.cep50_m, m.cep90_m);
  }
  std::printf("}\n");
  return 0;
}

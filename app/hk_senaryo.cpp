// Duyarlılık tablosu, yörünge senaryoları ve haritayı üreten program.
//   hk_senaryo [cikti_klasoru]
// Üretilenler: duyarlilik.md, senaryo_*.csv, harita.html
#include <cstdio>
#include <string>

#include "hk/harita.hpp"
#include "sentetik_arazi.hpp"

using namespace hk;

static void yaz_mc(std::FILE* f, const char* ad, const McSonuc& m) {
  std::fprintf(f, "| %-12s | %8.1f | %8.1f | %8.1f | %+7.1f | %+7.1f |\n", ad, m.cep50_m, m.cep90_m,
               m.rms_m, m.kuzey_bias_m, m.dogu_bias_m);
}

int main(int argc, char** argv) {
  const std::string klasor = argc > 1 ? argv[1] : "sonuclar";
  const Kamera kam = kamera_fovdan(1280, 720, derece2rad(30));  // orta zoom EO kamera
  const FonksiyonDem dem = sentetik_arazi();
  const Belirsizlik b = ornek_belirsizlik();
  const int N = 2000;

  // --- 1) Duyarlılık: iki geometri, nadire yakın ve eğik bakış -------------
  std::string md = "# Duyarlılık analizi (sentetik)\n\n";
  md += "Kamera 1280x720, HFOV 30°. Hedef sentetik tepelik arazide. Her satır yalnız o hata\n"
        "kaynağı açıkken 2000 örneklik Monte Carlo. Birim: metre (yatay).\n\n";
  struct Geo { const char* ad; double agl, yatay; };
  const Geo geolar[] = {{"Dik bakış (AGL 800 m, yatay 300 m)", 800, 300},
                        {"Eğik bakış (AGL 800 m, yatay 3000 m)", 800, 3000}};
  for (const Geo& gg : geolar) {
    LLA hedef = lla_derece(ARAZI_LAT0, ARAZI_LON0, 0);
    hedef.h_m = dem.yukseklik(hedef.lat_rad, hedef.lon_rad);
    Gozlem g;
    g.iha = ecef_to_lla(ned_to_ecef({-gg.yatay, 0, -gg.agl}, hedef));  // hedefin güneyinde
    g.govde = euler_derece(0, 0, 0);                                    // kuzeye uçuyor
    gimbali_yonelt(g.iha, g.govde, hedef, g.pan_rad, g.tilt_rad);
    g.u = kam.cx;
    g.v = kam.cy;
    const double menzil = std::hypot(gg.agl, gg.yatay);
    char bas[256];
    std::snprintf(bas, sizeof bas, "## %s\n\nEğik menzil %.0f m, gimbal tilt %.1f°.\n\n", gg.ad,
                  menzil, rad2derece(g.tilt_rad));
    md += bas;
    md += "| kaynak | CEP50 | CEP90 | RMS | kuzey yanlılık | doğu yanlılık |\n";
    md += "|---|---:|---:|---:|---:|---:|\n";
    std::printf("\n%s (menzil %.0f m)\n", gg.ad, menzil);
    for (const Katki& k : duyarlilik(kam, g, dem, b, N, 42)) {
      yaz_mc(stdout, k.kaynak.c_str(), k.sonuc);
      char sat[256];
      std::snprintf(sat, sizeof sat, "| %s | %.1f | %.1f | %.1f | %+.1f | %+.1f |\n",
                    k.kaynak.c_str(), k.sonuc.cep50_m, k.sonuc.cep90_m, k.sonuc.rms_m,
                    k.sonuc.kuzey_bias_m, k.sonuc.dogu_bias_m);
      md += sat;
    }
    md += "\n";
  }

  // --- 2) Yörünge senaryoları: rastgele hata, + sabit yanlılık -------------
  SenaryoAyar a;
  a.hedef = lla_derece(ARAZI_LAT0 + 0.003, ARAZI_LON0 - 0.002, 0);
  a.rastgele = b;
  a.sure_s = 377;  // 25 m/s, 1500 m yarıçap: tam tur ~377 s
  const SenaryoSonuc s1 = senaryo_calistir(kam, dem, a);

  SenaryoAyar a2 = a;
  a2.sabit.yaw_rad = derece2rad(1.0);   // pusula 1° kaçık
  a2.sabit.tilt_rad = derece2rad(0.3);  // gimbal sıfırı 0,3° kaçık
  a2.sabit.dem_m = 8.0;                 // DEM 8 m yüksek
  const SenaryoSonuc s2 = senaryo_calistir(kam, dem, a2);

  SenaryoAyar a3 = a2;
  a3.sure_s = 40;  // sadece 40 s: yayın küçük bir kısmı
  const SenaryoSonuc s3 = senaryo_calistir(kam, dem, a3);

  md += "# Yörünge senaryoları\n\n";
  md += "İHA hedef etrafında 1500 m yarıçap, 800 m AGL, 25 m/s daire çiziyor; gimbal\n"
        "hedefi ±40 piksel salınımla takip ediyor. Tek kare hata dağılımı ile, karelerin\n"
        "ortalaması ve çok bakışlı üçgenleme (DEM'siz) karşılaştırılıyor.\n\n";
  md += "| senaryo | kare | tek kare CEP50 | tek kare CEP90 | ortalama hatası | üçgenleme hatası |\n";
  md += "|---|---:|---:|---:|---:|---:|\n";
  struct Satir { const char* ad; const SenaryoSonuc* s; };
  const Satir satirlar[] = {{"rastgele hata, tam tur", &s1},
                            {"+ sabit yanlılık, tam tur", &s2},
                            {"+ sabit yanlılık, 40 s yay", &s3}};
  std::printf("\nYorunge senaryolari\n");
  for (const Satir& st : satirlar) {
    char sat[256];
    std::snprintf(sat, sizeof sat, "| %s | %zu | %.1f | %.1f | %.1f | %.1f |\n", st.ad,
                  st.s->kareler.size(), st.s->kare_cep50_m, st.s->kare_cep90_m,
                  st.s->ortalama_hata_m, st.s->ucgen_hata_m);
    md += sat;
    std::printf("%s", sat);
  }
  md += "\nSabit yanlılık: yaw +1°, gimbal tilt +0,3°, DEM +8 m.\n";

  bool ok = dosyaya_yaz(klasor + "/duyarlilik.md", md);
  ok &= dosyaya_yaz(klasor + "/senaryo_rastgele.csv", kareler_csv(s1));
  ok &= dosyaya_yaz(klasor + "/senaryo_yanlilik.csv", kareler_csv(s2));
  ok &= dosyaya_yaz(klasor + "/harita.html",
                    harita_html("Hedef konumlama: sentetik yörünge",
                                {{"rastgele hata", "#2b8cff", &s1},
                                 {"+ sabit yanlılık", "#ff5a36", &s2},
                                 {"+ yanlılık, 40 s", "#22b573", &s3}}));
  if (!ok) {
    std::fprintf(stderr, "cikti yazilamadi: %s/ klasoru var mi?\n", klasor.c_str());
    return 1;
  }
  std::printf("\nYazildi: %s/duyarlilik.md, harita.html, senaryo_*.csv\n", klasor.c_str());
  return 0;
}

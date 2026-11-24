#pragma once
// Sonucu göstermek. Geolocation çıktısı bir sayı listesi değil,
// haritada bir nokta + belirsizlik çemberidir. Tek başına açılan bir HTML
// (Leaflet + OpenStreetMap) ve tablo işleri için CSV üretiyoruz.
#include <string>
#include <vector>

#include "hk/senaryo.hpp"

namespace hk {

struct HaritaKatman {
  std::string ad;           // açıklamada görünen isim
  std::string renk;         // "#rrggbb"
  const SenaryoSonuc* s;
};

// Bir veya birkaç senaryoyu aynı haritada gösterir.
std::string harita_html(const std::string& baslik, const std::vector<HaritaKatman>& katmanlar);

std::string kareler_csv(const SenaryoSonuc& s);

bool dosyaya_yaz(const std::string& yol, const std::string& icerik);

}  // namespace hk

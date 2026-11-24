#include "hk/harita.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace hk {

static std::string nokta_json(const LLA& p) {
  char b[64];
  std::snprintf(b, sizeof b, "[%.8f,%.8f]", rad2derece(p.lat_rad), rad2derece(p.lon_rad));
  return b;
}

std::string harita_html(const std::string& baslik, const std::vector<HaritaKatman>& katmanlar) {
  std::ostringstream js;
  js << "[";
  for (size_t i = 0; i < katmanlar.size(); ++i) {
    const HaritaKatman& k = katmanlar[i];
    const SenaryoSonuc& s = *k.s;
    js << (i ? "," : "") << "{\"ad\":\"" << k.ad << "\",\"renk\":\"" << k.renk << "\",";
    js << "\"hedef\":" << nokta_json(s.hedef) << ",";
    js << "\"cep50\":" << s.kare_cep50_m << ",\"cep90\":" << s.kare_cep90_m << ",";
    js << "\"ort\":" << nokta_json(s.ortalama) << ",\"ortHata\":" << s.ortalama_hata_m << ",";
    js << "\"ucgen\":" << (s.ucgen_ok ? nokta_json(s.ucgen) : "null") << ",\"ucgenHata\":"
       << s.ucgen_hata_m << ",";
    js << "\"iz\":[";
    bool ilk = true;
    for (const Kare& kr : s.kareler) { js << (ilk ? "" : ",") << nokta_json(kr.iha); ilk = false; }
    js << "],\"tahmin\":[";
    ilk = true;
    for (const Kare& kr : s.kareler)
      if (kr.bulundu) { js << (ilk ? "" : ",") << nokta_json(kr.tahmin); ilk = false; }
    js << "]}";
  }
  js << "]";

  std::ostringstream h;
  h << R"HTML(<!doctype html>
<html lang="tr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>)HTML" << baslik << R"HTML(</title>
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css">
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<style>
  html,body{margin:0;height:100%;font:14px system-ui,sans-serif;background:#111;color:#eee}
  #harita{position:absolute;inset:0}
  #panel{position:absolute;top:10px;right:10px;z-index:1000;background:rgba(17,17,17,.9);
         padding:10px 14px;border-radius:8px;max-width:340px}
  #panel h1{font-size:15px;margin:0 0 6px}
  #panel table{border-collapse:collapse;width:100%}
  #panel td{padding:2px 6px;border-top:1px solid #333}
  .kutu{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:6px}
</style></head><body>
<div id="harita"></div><div id="panel"><h1>)HTML" << baslik << R"HTML(</h1><div id="tablo"></div>
<p style="margin:8px 0 0;color:#aaa;font-size:12px">Küçük noktalar: tek kare tahminleri.
Çember: tek kare CEP50 / CEP90. Kare: kareler ortalaması. Üçgen: çok bakışlı üçgenleme.
Siyah artı: gerçek hedef. Veri sentetiktir.</p></div>
<script>
const K = )HTML" << js.str() << R"HTML(;
const m = L.map('harita');
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
  {maxZoom: 19, attribution: '&copy; OpenStreetMap'}).addTo(m);
let sinir = [];
let satir = '<table><tr><td></td><td>CEP50</td><td>ort.</td><td>üçgen</td></tr>';
for (const k of K) {
  L.polyline(k.iz, {color: k.renk, weight: 1.5, opacity: .6}).addTo(m);
  for (const p of k.tahmin) L.circleMarker(p, {radius: 2, color: k.renk, weight: 0, fillOpacity: .5}).addTo(m);
  L.circle(k.hedef, {radius: k.cep50, color: k.renk, weight: 2, fill: false}).addTo(m);
  L.circle(k.hedef, {radius: k.cep90, color: k.renk, weight: 1, dashArray: '4 4', fill: false}).addTo(m);
  L.marker(k.ort, {icon: L.divIcon({className: '', html:
    `<div style="width:10px;height:10px;background:${k.renk};border:2px solid #000"></div>`})}).addTo(m);
  if (k.ucgen) L.marker(k.ucgen, {icon: L.divIcon({className: '', html:
    `<div style="width:0;height:0;border-left:7px solid transparent;border-right:7px solid transparent;border-bottom:12px solid ${k.renk}"></div>`})}).addTo(m);
  sinir = sinir.concat(k.tahmin);  // görünüm: tahmin dağılımı (yörünge 1,5 km dışarıda)
  satir += `<tr><td><span class="kutu" style="background:${k.renk}"></span>${k.ad}</td>` +
           `<td>${k.cep50.toFixed(1)} m</td><td>${k.ortHata.toFixed(1)} m</td>` +
           `<td>${k.ucgen ? k.ucgenHata.toFixed(1) + ' m' : '-'}</td></tr>`;
}
document.getElementById('tablo').innerHTML = satir + '</table>';
L.marker(K[0].hedef, {icon: L.divIcon({className: '', html:
  '<div style="font:bold 22px monospace;color:#000;transform:translate(-4px,-12px)">+</div>'})}).addTo(m);
const sigdir = () => { m.invalidateSize(); m.fitBounds(sinir, {padding: [30, 30]}); };
m.setView(K[0].hedef, 13);  // kapsayıcı henüz boyutsuzsa bile geçerli bir görünüm
window.addEventListener('load', sigdir);
window.addEventListener('resize', sigdir);
setTimeout(sigdir, 50);
</script></body></html>
)HTML";
  return h.str();
}

std::string kareler_csv(const SenaryoSonuc& s) {
  std::ostringstream o;
  o << "t_s,iha_lat,iha_lon,iha_h,yaw_deg,u,v,bulundu,tahmin_lat,tahmin_lon,tahmin_h,hata_m\n";
  char b[256];
  for (const Kare& k : s.kareler) {
    std::snprintf(b, sizeof b, "%.1f,%.8f,%.8f,%.2f,%.3f,%.2f,%.2f,%d,%.8f,%.8f,%.2f,%.3f\n", k.t_s,
                  rad2derece(k.iha.lat_rad), rad2derece(k.iha.lon_rad), k.iha.h_m,
                  rad2derece(k.yaw_rad), k.u, k.v, k.bulundu ? 1 : 0,
                  rad2derece(k.tahmin.lat_rad), rad2derece(k.tahmin.lon_rad), k.tahmin.h_m,
                  k.hata_m);
    o << b;
  }
  return o.str();
}

bool dosyaya_yaz(const std::string& yol, const std::string& icerik) {
  std::ofstream f(yol, std::ios::binary);
  if (!f) return false;
  f << icerik;
  return static_cast<bool>(f);
}

}  // namespace hk

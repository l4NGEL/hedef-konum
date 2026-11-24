# 8. Gösterim ve bağlantı noktaları

**Kod:** [harita.hpp](../include/hk/harita.hpp), [harita.cpp](../src/harita.cpp), [hk_coz.cpp](../app/hk_coz.cpp) · **Çıktı:** [sonuclar/harita.html](../sonuclar/harita.html)

## Harita
Geolocation'ın çıktısı bir sayı listesi değil: **haritada bir nokta ve onun belirsizlik çemberi**. `harita_html` tek dosyalık bir sayfa üretiyor (Leaflet + OpenStreetMap):
- ince çizgi: İHA yörüngesi
- küçük noktalar: tek kare tahminleri
- düz/kesikli çember: tek kare CEP50/CEP90 (gerçek hedefin çevresinde)
- kare işaret: kareler ortalaması · üçgen işaret: üçgenleme · siyah artı: gerçek hedef

Altlık gerçek OSM haritası; arazi ve senaryo sentetik (panelde de yazıyor).

**Tarayıcıda karşılaşılan bir hata:** İlk sürümde harita en yüksek zoom'da açılıyordu. Sebebi, `fitBounds` çağrıldığında kapsayıcının boyutunun henüz sıfır olmasıydı. Çözüm: sayfa yüklendikten sonra `invalidateSize()` + `fitBounds`. Görünüm de yörüngeye değil tahmin dağılımına sığdırılıyor; yoksa 3 km'lik bir dairede 20 m'lik dağılım tek bir nokta kadar kalıyordu.

## Komut satırı: `hk_coz`
Tek bir gözlemi çözüp JSON yazıyor. Tespitçiye (örneğin iha-takip) bağlanacak arayüz bu:

```powershell
.\build\Release\hk_coz.exe --iha 39.94 32.70 1900 --govde 0 0 0 --gimbal 0 -30 --piksel 639.5 359.5 --kamera 1280 720 30 --zemin 1000 --mc 1000
```
```json
{"bulundu":true,"lat":39.95404018,"lon":32.70000000,"h":1000.00,"menzil_m":1800.4,"cep50_m":19.4,"cep90_m":35.0}
```
Elle kontrol: 900 m yükseklikten 30° aşağı → yatay mesafe 900/tan 30° = 1559 m kuzey → +0,01404° enlem. ✓

`--dem N40E032.hgt` verilirse gerçek SRTM karosuyla ışın yürütme yapıyor (jeoid notu için bölüm 5'e bak).

## Bağlantı noktaları (sonraki adımlar)
| Ne | Nereden | Ne gerekiyor |
|---|---|---|
| piksel `(u, v)` | iha-takip tespit/takip kutusu | kutu merkezi; kutunun alt orta noktası araçlar için daha doğru (temas noktası) |
| İHA konumu, yönelim | MAVLink `GLOBAL_POSITION_INT`, `ATTITUDE` | video karesinin zaman damgasına ara değerle getirme |
| gimbal | MAVLink `GIMBAL_DEVICE_ATTITUDE_STATUS` | gövdeye mi yere mi göre olduğunu gösteren bayrak (bölüm 2) |
| gösterim | yer-istasyonu-qt haritası | `hk_core` kütüphanesini doğrudan bağlamak; JSON da yeterli |

## Sorular
1. Araç tespitinde neden kutu merkezi yerine alt orta nokta daha iyi?
2. CEP çemberini gerçek hedefin değil tahminin çevresine çizseydik ne ifade ederdi?

---
<details><summary>Cevaplar</summary>

1. Kutu merkezi aracın *gövdesinin ortası*, yani yerden ~1 m yukarıda bir nokta. Işın bu noktadan geçip daha arkadaki zemine çarpar. Eğik bakışta 1 m yükseklik farkı `1/tanθ` kadar menzil hatası demek (10°'de ~6 m). Alt orta nokta ise aracın yere değdiği yer.
2. Sahada gerçek hedef bilinmediği için operatöre gösterilecek şey o: "tahmin burada, %50 olasılıkla şu çemberin içinde". Bizim haritamız doğrulama amaçlı; çemberler, ölçülen dağılımın gerçek hedefe göre ne kadar geniş olduğunu gösteriyor.
</details>

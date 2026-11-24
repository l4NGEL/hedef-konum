# hedef-konum

İHA kamerasında görülen bir hedefin **haritadaki konumunu** ve bu konumun **belirsizliğini** hesaplayan C++17 kütüphanesi ve araçları.

Girdiler: hedefin piksel konumu, gimbal açıları, İHA'nın GPS konumu ve yönelimi, kamera parametreleri, sayısal yükseklik modeli (DEM).
Çıktılar: enlem/boylam/yükseklik, eğik menzil, CEP50/CEP90.

```
piksel ─► kamera ışını ─► NED ışını ─► ECEF ışını ─► DEM ile ışın yürütme ─► hedef LLA
 (iç parametre,  (gövde · gimbal ·    (WGS84)        (örtülmeyi yakalar)      + Monte Carlo CEP
  lens bozulması)  kamera dönüşleri)
```

Vektör/matris işleri, koordinat dönüşümleri ve DEM okuma dahil her şey sıfırdan yazıldı. Dış bağımlılık yok (testler için yalnız GoogleTest).

## Sonuçlar (sentetik veri)

**Tek gözlem, hata bütçesi** (GPS 2/4 m, roll/pitch 0,2°, yaw 0,5°, gimbal 0,1°, piksel 2, DEM 5 m; 2000 örneklik Monte Carlo):

| geometri | eğik menzil | CEP50 | CEP90 | en büyük kaynak |
|---|---:|---:|---:|---|
| dik bakış | 854 m | 5,1 m | 9,6 m | GPS yatay, roll, pitch |
| eğik bakış | 3105 m | 31,4 m | 58,6 m | yaw (18,9 m), pitch (14,3 m) |

**Hedef etrafında yörünge** (1500 m yarıçap, 800 m AGL, 378 kare):

| senaryo | tek kare CEP50 | kareler ortalaması | çok bakışlı üçgenleme |
|---|---:|---:|---:|
| rastgele hata | 20,6 m | 1,4 m | 2,1 m |
| + sabit yanlılık (yaw 1°, tilt 0,3°, DEM 8 m) | 35,1 m | 8,8 m | 2,0 m |
| + sabit yanlılık, yalnız 40 s yay | 42,0 m | 23,9 m | 24,4 m |

Ayrıntı: [sonuclar/duyarlilik.md](sonuclar/duyarlilik.md). Harita: `sonuclar/harita.html`.

Öne çıkan bulgular:
- Sığ açıda (tilt −5°, 23 km) düz zemin varsayımı hedefi ~490 m yakına koyuyor. Bu, `d²/(2R·tanθ)` formülüyle örtüşüyor.
- Yinelemeli DEM yöntemi bir sırtın arkasına "düşüyor"; ışın yürütme sırta çarpıyor. Görülen şey sırt olduğu için doğru cevap da bu.
- Tam yörüngede yaw yanlılığı düz zeminde neredeyse tamamen sönüyor. Tepelik arazide tam sönmüyor, çünkü yana kayma yükseklik farkı üzerinden menzil kaymasına dönüşüyor.
- Üçgenleme DEM kullanmadığı için DEM yanlılığından etkilenmiyor. Kısa yayda ise ışınlar neredeyse paralel kaldığı için zayıf.

## Derleme ve test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release        # 56 test
./build/hk_senaryo sonuclar              # Windows: build\Release\hk_senaryo.exe
```

Tek gözlem çözümü:
```bash
hk_coz --iha 39.94 32.70 1900 --govde 0 0 0 --gimbal 0 -30 --piksel 639.5 359.5 \
       --kamera 1280 720 30 --zemin 1000 --mc 1000
# {"bulundu":true,"lat":39.95404018,"lon":32.70000000,"h":1000.00,"menzil_m":1800.4,"cep50_m":19.4,"cep90_m":35.0}
```
`--dem N40E032.hgt` ile SRTM karosu kullanılabilir.

## Yapı

| modül | içerik | not |
|---|---|---|
| `geodezi` | WGS84, LLA ↔ ECEF ↔ NED | [not 1](docs/01_koordinatlar.md) |
| `donus` | ZYX Euler, gövde/gimbal/kamera zinciri | [not 2](docs/02_donusler.md) |
| `kamera` | pinhole, k1/k2 radyal bozulma, piksel ↔ ışın | [not 3](docs/03_kamera.md) |
| `geolokasyon` | düz zemin, elipsoid kesişimi | [not 4](docs/04_zemin_kesisimi.md) |
| `dem` | SRTM .hgt okuma, bilinear, ışın yürütme, yinelemeli yöntem | [not 5](docs/05_dem.md) |
| `belirsizlik` | Monte Carlo, CEP, duyarlılık analizi | [not 6](docs/06_hata_butcesi.md) |
| `senaryo` | ileri model, gimbal yönlendirme, yörünge simülasyonu, üçgenleme | [not 7](docs/07_dogrulama.md) |
| `harita` | Leaflet HTML, CSV | [not 8](docs/08_gosterim.md) |

`docs/` altındaki notlar her modülün arkasındaki matematiği, tuzakları ve soru-cevapları içeriyor.

## Sınırlar
- Arazi, uçuş ve hata bütçesi **sentetik**. Gerçek uçuş verisi, SITL ve gerçek video kullanılmadı. Hata sigmaları bir kaynaktan değil, makul varsayımdan alındı.
- DEM'in elipsoid yüksekliği verdiği varsayılıyor. Gerçek SRTM, EGM96 jeoidine göre; jeoid düzeltmesi eklenmedi.
- Atmosferik kırılma, kamera montaj hatası, telemetri ile video arasındaki zaman kayması ve zamanla ilişkili IMU hataları modellenmedi.
- Hedef duran hedef olarak varsayılıyor; hareketli hedef için bir filtre yok.

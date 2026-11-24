# 5. Gerçek arazi: DEM ve ışın yürütme

**Kod:** [dem.hpp](../include/hk/dem.hpp), [dem.cpp](../src/dem.cpp) · **Test:** [test_dem.cpp](../tests/test_dem.cpp)

## Amaç
Arazi düz değil. Hedef bir tepe yamacında ya da vadi dibinde olabilir. DEM (Sayısal Yükseklik Modeli) her enlem/boylam için zeminin yüksekliğini verir; ışını bu yüzeyle kesiştiriyoruz.

## DEM
- **SRTM:** açık, ~30 m (1") veya ~90 m (3") çözünürlüklü, mutlak doğruluğu ~5–10 m. Karolar 1°×1°, dosya adı `N40E032.hgt` (sol alt köşe). İçerik büyük-endian `int16`, satır 0 en kuzey, `−32768` = veri yok.
- **Komşu karolar kenarı paylaşır:** 1201 örnek 1200 aralık demek, adım `1/(n−1)` derece. `n` alırsan yarım hücre kayarsın (~45 m).
- **Çift doğrusal ara değer:** çevredeki 4 hücrenin ağırlıklı ortalaması. Köşelerden biri boşsa sonuç bilerek NaN.
- `IzgaraDem::hgt_oku` gerçek SRTM dosyalarını okuyabiliyor. Testte sentetik bir `.hgt` dosyası yazıp doğruluyoruz; gerçek karo indirmedik.

> **Yükseklik referansı.** SRTM yükseklikleri EGM96 jeoidine göre, GPS'inki WGS84 elipsoidine göre. Türkiye'de jeoid ≈ +30…+40 m. Gerçek SRTM ile çalışırken her noktaya jeoid ondülasyonu eklemek gerekir. Yapılmazsa eğik bakışta (`tanθ` küçük) bu fark yüzlerce metre menzil hatasına dönüşür. Bu proje DEM'in elipsoid yüksekliği verdiğini varsayıyor.

## İki yöntem

### A) Işın yürütme (ray marching): `dem_isin_yurut`
Işın boyunca `adim_m` aralıklarla ilerle ve her noktada "zeminin üstünde miyim?" diye bak. İşaret değiştiği aralığı ikiye bölerek (bisection) 0,1 mm'ye kadar daralt.
- **Örtülmeyi doğru yakalar:** önde bir sırt varsa ilk çarptığı yer sırttır. Fiziksel olarak doğru cevap da bu: görülen şey sırt, hedef görünmüyor.
- **Adım seçimi:** adım, aradaki en ince engelden kısa olmalı. `BuyukAdimInceSirtiKacirir` testi: 30 m kalınlığındaki bir sırtı 70 m adım ıskalıyor. 100 m adım ise *tesadüfen* yakaladı, çünkü bir örnek sırtın üstüne düştü. Sonucun örneğin nereye düştüğüne bağlı olması örtüşmenin (aliasing) tanımı. Varsayılan 10 m; SRTM çözünürlüğü 30 m olduğu için yeterli.

### B) Yinelemeli düz yükseklik: `dem_yinelemeli`
`h0` ile kesiştir → oradaki DEM yüksekliğini oku → onunla tekrar kesiştir. Hızlı ve yumuşak arazide iyi çalışıyor, birçok sistemde kullanılıyor. **Ama:**
- Örtülmeyi hiç anlamıyor. `TepeArkasiOrtulme` testinde ışın 2 km'deki sırta çarpıyor; yinelemeli yöntem sırtın arkasındaki düzlüğe "düşüp" 3,5 km diyor.
- Dik arazide ve sığ açıda salınabilir. Kod bu durumda `bulundu = false` döndürüyor.

## Sorular
1. 3" SRTM'de bir hücre kuzey-güney yönünde kaç metre? Doğu-batı yönünde, 40° enlemde?
2. DEM 5 m hatalı. Tilt −10°'de bu kaç metre menzil hatası yapar? Tilt −60°'de?
3. Işın yürütmede adımı `zemin_ustu / 2` gibi uyarlamalı seçmek cazip. Hangi durumda tehlikeli?

---
<details><summary>Cevaplar</summary>

1. 3" = 1/1200° → kuzey-güney ≈ 92,6 m; doğu-batı ≈ 92,6·cos 40° ≈ 71 m.
2. Menzil yönündeki yatay hata ≈ `Δh / tan|θ|`: −10° → ~28 m, −60° → ~2,9 m. Eğik bakışın kötü olmasının sebeplerinden biri bu (bölüm 6).
3. Işın, yamacı boyunca yükselen ve ışına paralel giden bir sırtın yanından geçiyorsa. "Ne kadar yukarıdayım" bilgisi, önümde ne kadar *ileride* engel olduğunu söylemez. Sığ açıda adım çok büyür ve ince engeller kaçar.
</details>

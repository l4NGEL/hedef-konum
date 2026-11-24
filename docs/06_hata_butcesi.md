# 6. Hata bütçesi: Monte Carlo ve CEP

**Kod:** [belirsizlik.hpp](../include/hk/belirsizlik.hpp), [belirsizlik.cpp](../src/belirsizlik.cpp) · **Test:** [test_belirsizlik.cpp](../tests/test_belirsizlik.cpp) · **Sonuç:** [sonuclar/duyarlilik.md](../sonuclar/duyarlilik.md)

## Amaç
"Hedef şu koordinatta" demek yetmez. Asıl soru şu: **ne kadar emin olabiliriz?** Her girdinin bir hatası var: GPS, IMU, gimbal enkoderleri, tespit kutusu, DEM. Bu hataların sonuca nasıl yansıdığını bulmamız gerekiyor.

## Yöntem
1. Hatasız gözlemi çöz → "gerçek" nokta.
2. Her girdiyi kendi 1-sigma'sı kadar rastgele boz (`gozlemi_boz`), tekrar çöz. Bunu 2000 kez yap.
3. Hataları gerçek noktanın NED düzleminde ölç ve dağılıma bak:
   - **CEP50:** sonuçların yarısının içine düştüğü çember. **CEP90:** %90'ı.
   - **Yanlılık:** ortalama hata vektörü (hep aynı yöne kayma).
4. **Duyarlılık:** kaynakları tek tek aç; her biri toplamın ne kadarını oluşturuyor?

Aynı tohum aynı sonucu veriyor (`TekrarlanabilirTohum`), yani deneyler tekrarlanabilir.

> GPS hatası NED'de metre olarak ekleniyor, enleme "derece" olarak değil. Enlemi derece cinsinden bozmak, metre karşılığını enleme göre değiştirirdi.

## Varsayılan hata bütçesi (`ornek_belirsizlik`)
GPS 2 m yatay / 4 m düşey · roll/pitch 0,2° · yaw 0,5° · gimbal 0,1° · piksel 2 · DEM 5 m. Bunlar bir kaynaktan alınmış değerler değil, makul varsayımlar.

## Sonuçlar (sentetik tepelik arazi, HFOV 30°)

| | dik bakış (854 m menzil) | eğik bakış (3105 m menzil) |
|---|---:|---:|
| en büyük kaynak | GPS yatay / roll / pitch (~2 m) | **yaw 18,9 m**, pitch 14,3 m |
| hepsi birden CEP50 | **5,1 m** | **31,4 m** |
| hepsi birden CEP90 | 9,6 m | 58,6 m |

Tam tablo: [sonuclar/duyarlilik.md](../sonuclar/duyarlilik.md).

## Okuma
- **Dik bakışta** açı hataları kısa menzille çarpıldığı için küçük kalıyor; GPS'in kendi 2 m'si baskın hale geliyor.
- **Eğik bakışta** açı hataları uzun menzille çarpılıyor:
  - yaw → yanal hata ≈ `yatay mesafe × σ`
  - pitch/tilt → menzil hatası ≈ `h / sin²θ × σ`
  - GPS düşey ve DEM → menzil hatası ≈ `Δh / tanθ`

  Bu yüzden eğik bakışta GPS düşey hatası (4,9 m), GPS yatay hatasını (1,8 m) geçiyor.
- **Eğimin etkisi:** Bu senaryoda hedef kameraya bakan bir yamaçta. Yamaç ışına daha dik durduğu için pitch/DEM/GPS düşey hatalarının etkisi düz zemin formülünün verdiğinden küçük çıkıyor (pitch'te formül ~28 m diyor, ölçülen 14 m). Düz zeminde formül tutuyor: `EgikBakistaDemHatasiMenzilYonunde` testi 10 m DEM hatasının yaklaşık 60 m menzil kaymasına dönüştüğünü doğruluyor.
- **Nadirde yaw etkisiz** (`NadirdeYawHatasiEtkisiz`): dümdüz aşağı bakarken merkez piksel dikey eksenin üzerinde; yaw onu kendi etrafında döndürüyor ama yerinden oynatmıyor.

**Mühendislik sonucu:** Eğik bakışta en çok kazandıracak yatırım yaw doğruluğu (çift anten GPS ile yön bulma ya da daha iyi bir AHRS). Dik bakışta ise daha iyi GPS (RTK).

## Sorular
1. Yaw σ'sını 0,5°'den 0,25°'ye indirsek eğik bakıştaki toplam CEP50 yaklaşık kaç olur?
2. CEP50 ile RMS neden farklı? Hangisi aykırı değerlere daha duyarlı?
3. Neden tek bir "en kötü durum" hesabı yerine Monte Carlo yapıyoruz?

---
<details><summary>Cevaplar</summary>

1. Bağımsız hatalar karesel toplanır. Yaw katkısı ~19 m'den ~9,5 m'ye iner; diğerlerinin karesel toplamı ≈ √(31,4² − 18,9²) ≈ 25 m. Yeni toplam ≈ √(25² + 9,5²) ≈ **27 m**. Yani yaw'ı yarıya indirmek toplamı sadece ~%14 iyileştirir; sıradaki darboğaz pitch olur.
2. CEP50 bir medyan, RMS ise kareler ortalaması. RMS aykırı değerlere çok daha duyarlı. Dağılım kuyruklu olduğunda (sığ açı, arazi) RMS büyür.
3. Hatalar doğrusal olmayan bir zincirden geçiyor (arazi, örtülme, sığ açı). Analitik bir "en kötü durum" ya aşırı kötümser olur ya da arazi etkisini kaçırır. Monte Carlo ise zincirin kendisini çalıştırdığı için bu etkileri doğrudan içeriyor.
</details>

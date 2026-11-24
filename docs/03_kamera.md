# 3. Pinhole kamera: pikselden ışına

**Kod:** [kamera.hpp](../include/hk/kamera.hpp), [kamera.cpp](../src/kamera.cpp) · **Test:** [test_kamera.cpp](../tests/test_kamera.cpp)

## Amaç
Tespitçi (örneğin iha-takip) bir kutu verir; kutu merkezi `(u, v)` pikseli. Bu adımın işi o pikseli kamera çerçevesinde bir **yön** vektörüne çevirmek. Bölüm 2'deki `R_ned_kam` ile çarpınca ışının NED'deki yönü çıkar.

## Kavramlar
1. **İzdüşüm:** `x' = X/Z, y' = Y/Z` → bozulma → `u = fx·x'' + cx`, `v = fy·y'' + cy`. Geolocation için bunun tersini yapıyoruz.
2. **`fx` görüş açısından:** `tan(HFOV/2) = (w/2) / fx`. Zoom değiştikçe `fx` de değişir. Zoom kademesini bilmezsen hedef zoom yaptıkça görüntü merkezine doğru "kayar".
3. **Kamera derinliği bilemez.** `X/Z` bölmesi uzaklığı yok eder. Elimizde bir nokta değil bir ışın var; uzaklığı bölüm 4–5'te zemin buluyor.
4. **Radyal bozulma (Brown-Conrady):** `ölçek = 1 + k1·r² + k2·r⁴`. Tersi kapalı formda yok; sabit nokta yinelemesiyle çözülüyor. `k1 < 0` fıçı bozulması demek, geniş açılı lenslerde tipik.
5. **Piksel merkezi `(w − 1)/2`:** 1280 piksellik görüntüde orta nokta 639,5. Bunu 640 almak yarım piksellik sistematik bir kayma yaratır.

## Tuzaklar
- `BozulmayiYokSaymak` testi: köşedeki piksel 2°'den fazla sapıyor, bu da 1 km'de 35 m'den fazla hata demek. Gerçek kamerada `k1, k2` değerleri OpenCV satranç tahtası kalibrasyonuyla bulunur. Kalibrasyon zoom'a bağlıysa her kademe için ayrı yapılmalı.
- Termal kameralar genelde daha dar açılı ve daha düşük çözünürlüklüdür. 640 piksellik bir termal kamerada 1 piksel, aynı HFOV'deki EO kameranın iki katı açıya karşılık gelir.

## Sorular
1. 1280 piksel genişlik, HFOV 30°. `fx` kaçtır? Bir piksel kaç derece eder?
2. `piksel_to_isin` neden birim vektör döndürüyor? Döndürmeseydi bölüm 4'te ne bozulurdu?
3. Görüntü merkezindeki pikselde bozulma katsayılarının etkisi nedir?

---
<details><summary>Cevaplar</summary>

1. `fx = 640 / tan 15° ≈ 2388`. Merkezde bir piksel ≈ `1/2388` rad ≈ 0,024°; 3 km menzilde ~1,3 m.
2. Bölüm 4'teki `t` (ışın parametresi) doğrudan metre cinsinden **menzil** olsun diye. Birim değilse `menzil_m` yanlış çıkar ve DEM yürütme adımı metre olmaktan çıkar.
3. Hiçbir etkisi yok: `r = 0` olduğu için ölçek 1. Bozulma merkezden uzaklaştıkça büyür. Takip sırasında hedefi merkezde tutmak bu yüzden de iyi bir şey.
</details>

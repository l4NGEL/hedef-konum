# 1. Dünya üzerindeki koordinat sistemleri

**Kod:** [geodezi.hpp](../include/hk/geodezi.hpp), [geodezi.cpp](../src/geodezi.cpp) · **Test:** [test_geodezi.cpp](../tests/test_geodezi.cpp)

## Neden önce koordinatlar?
Hedefi bulmak için gereken bilgiler (GPS konumu, gimbal açıları, piksel) farklı "dillerde" geliyor. Açılarla toplama/çıkarma yapılmaz; metreyle yapılır. Önce ortak bir dil gerekiyor.

| Sistem | Ne | Nerede kullanıyoruz |
|---|---|---|
| **LLA** | enlem, boylam, elipsoid yüksekliği | GPS bunu verir; sonucu da bununla veririz |
| **ECEF** | Dünya merkezli X, Y, Z (metre) | Işın + zemin kesişimi gibi tüm "küresel" hesaplar |
| **NED** | Kuzey, Doğu, Aşağı (metre), bir referans noktada | Uçağın çevresi; gimbal/kamera açıları buna göre |

## Kavramlar
1. **Dünya küre değil, basık.** WGS84 elipsoidi: ekvator yarıçapı `a = 6 378 137 m`, kutup yarıçapı `b ≈ 6 356 752 m` (~21 km fark).
2. **`N(φ)` (dik doğrultu eğrilik yarıçapı):** o enlemde elipsoide dik çizginin dönme eksenine kadar uzunluğu. `lla_to_ecef` içindeki `(1 − e²)` çarpanı Z'deki basıklığı getirir.
3. **ECEF → LLA kapalı formda yok**, enlemi yineleyerek buluyoruz. Kutupta `p/cos(φ)` sıfıra bölündüğü için yüksekliği `h = p·cosφ + z·sinφ − a·√(1 − e²sin²φ)` ile hesaplıyoruz.
4. **NED = önce ötele, sonra döndür.** Matrisin satırları referans noktadaki Kuzey, Doğu, Aşağı birim vektörleri. D aşağıyı gösterir: 50 m yukarı = `D = −50`.
5. **Ters dönüşüm (`ned_to_ecef`):** önce geri döndür (`R⁻¹ = Rᵀ`), sonra referansın ECEF konumunu ekle: `lla_to_ecef(ref) + Rᵀ·ned`. Test: `Ned.NedEcefGidisDonus`.
6. **Yön vektörü ≠ konum.** Bir *yönü* NED'e çevirirken öteleme yapılmaz, sadece döndürülür (`yon_ecef_to_ned`). Bölüm 4'te ışın yönü için bunu kullanıyoruz.

## Tuzaklar
- **Elipsoid yüksekliği ≠ deniz seviyesi.** GPS `h`'si elipsoide göre; haritalardaki rakım jeoide (≈ deniz seviyesi) göre. Türkiye'de fark +30…+40 m (bölüm 5).
- Enlem/boylamı radyan mı derece mi tuttuğunu her fonksiyonda bil. Bu projede içeride hep radyan, dışarıda derece (`lla_derece`).

## Sorular
1. 111 m kuzeydeki ve aynı yükseklikteki nokta neden NED'de `D > 0` çıkıyor?
2. ECEF'te iki nokta arası 10 km. NED'de de 10 km mi olur?
3. Dönme matrisinin tersi neden devriği?

---
<details><summary>Cevaplar</summary>

1. NED düzlemi referansta yere teğet; zemin uzaklaştıkça bu düzlemin `d²/2R` altına iner (111 m'de ~1 mm, 10 km'de ~8 m). Bölüm 4'teki "düz zemin bozulur" bulgusunun kaynağı bu.
2. Evet. Öteleme ve dönme uzunlukları korur; NED sadece eksenleri değiştirir, Dünya'yı "düzleştirmez".
3. Satırları birbirine dik birim vektörler olduğu için `R·Rᵀ = I`. Ters almak için bölme yok, hem hızlı hem tam doğru.
</details>

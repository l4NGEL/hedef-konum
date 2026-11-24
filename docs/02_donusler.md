# 2. Gövde, gimbal ve kamera dönüşleri

**Kod:** [donus.hpp](../include/hk/donus.hpp), [donus.cpp](../src/donus.cpp) · **Test:** [test_donus.cpp](../tests/test_donus.cpp)

## Amaç
Kameranın baktığı yön kamera çerçevesinde belli. Bizim bu yönü NED'de bilmemiz gerekiyor. Arada üç dönüş var:

```
R_ned_kam = R_ned_govde · R_govde_gimbal · R_gimbal_kam
             (İHA yönelimi)  (gimbal pan/tilt)  (eksen adlandırma)
```

## Kavramlar
1. **İsim kuralı.** `R_a_b`, b çerçevesindeki vektörü a çerçevesine çevirir: `v_a = R_a_b · v_b`. Zincirde içteki harfler birbirini götürür. Sahadaki geolocation hatalarının çoğu buradan çıkar; başkasının koduna bakarken ilk soracağın şey bu.
2. **Çerçeveler.** Gövde FRD (x burun, y sağ kanat, z aşağı). Gimbal: x optik eksen. Kamera (OpenCV): **z ileri**, x sağ, y aşağı. `R_gimbal_kam` hiçbir açı içermez, sadece eksenleri yeniden adlandırır.
3. **Sıra önemli.** Havacılık standardı ZYX: `Rz(yaw)·Ry(pitch)·Rx(roll)`. MAVLink `ATTITUDE` mesajı da bu kurala göre. `SiraOnemli` testi aynı açılarla farklı sırada çok farklı bir yön çıktığını gösteriyor.
4. **İşaretler.** roll + = sağ kanat aşağı, pitch + = burun yukarı, yaw + = kuzeyden doğuya. `Ry`'deki `sin` işaretlerinin "ters" görünmesi döngüsel sıradan (`z × x = y`) geliyor.
5. **Gimbal kilidi.** `matris_to_euler`, pitch ±90°'de yaw ile roll'u ayıramaz. Gimbal tilt −90° (dümdüz aşağı) çok sık kullanılır. Bu yüzden hesapları Euler ile değil matrisle yapıyoruz.

## Tuzaklar
- **Stabilize gimbal.** Bazı gimbal'ler açıyı gövdeye göre değil, *yere göre* verir. O zaman `R_ned_govde`'den roll ve pitch çıkarılır, yalnız yaw kalır. Bunu karıştırırsan roll'u iki kez hesaba katarsın. MAVLink `GIMBAL_DEVICE_ATTITUDE_STATUS` mesajında bunu belirten bir bayrak var.
- Kamera montaj açısı (gimbal ile kamera arasındaki küçük sabit eğiklik) gerçekte sıfır değildir. Sabit bir yanlılık gibi davranır (bölüm 6–7).

## Sorular
1. İHA kuzeye uçuyor, gimbal tilt −30°, roll +20°. Kamera ışını doğuya mı batıya mı kayar?
2. Bir dönme matrisinin determinantı kaçtır? −1 çıkarsa ne olmuştur?
3. `R_ned_kam`'ın devriği neyi yapar?

---
<details><summary>Cevaplar</summary>

1. **Batıya.** Sağ kanat aşağı inince gövdenin "aşağı" ekseni sola (batı) yatar; öne-aşağı bakan kamera da onunla batıya döner. Doğu bileşeni ≈ −0,17 olur; 1 km'de ~170 m.
2. +1. −1 ise matris bir yansıma içeriyor demektir; genelde bir ekseni ters tanımlamışsındır (örneğin sol el çerçevesi).
3. NED'deki bir yönü kamera çerçevesine çevirir. Bölüm 7'deki ileri model (`hedefi_izdusur`) tam olarak bunu kullanıyor.
</details>

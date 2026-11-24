# 4. Işını zeminle kesiştirmek

**Kod:** [geolokasyon.hpp](../include/hk/geolokasyon.hpp), [geolokasyon.cpp](../src/geolokasyon.cpp) · **Test:** [test_geolokasyon.cpp](../tests/test_geolokasyon.cpp)

## Amaç
Artık elimizde bir ışın var: başlangıcı İHA'nın ECEF konumu, yönü `isin_ecef`. Işının zemine çarptığı nokta hedef. Burada zemini iki basit modelle temsil ediyoruz. Gerçek arazi bölüm 5'te.

```
Gozlem { iha, govde, pan, tilt, u, v }
   → isin_ned = R_ned_kam · piksel_to_isin(u, v)
   → isin_ecef = R_ned_ecefᵀ · isin_ned          (yön: öteleme yok!)
   → zemin modeliyle kesişim → hedef LLA
```

## 1) Düz zemin
NED'de zemin, `D = h_iha − h_zemin` düzlemi. Işın `p(t) = t·d` olduğuna göre `t = D0 / d.z`. Dört satırlık bir hesap.
- `d.z ≤ 0` ise ışın ufka ya da göğe bakıyor demektir; kesişim yok. Kodda bunu açıkça kontrol ediyoruz.

## 2) Elipsoid
WGS84 elipsoidini `h` kadar şişirip `(x² + y²)/A² + z²/B² = 1` denklemine ışını koyuyoruz. Ortaya `t` cinsinden ikinci derece bir denklem çıkıyor; küçük pozitif kök ilk çarpma noktası.
- **Sayısal kararlılık:** `(−b ± √Δ)/2a` formülünde `b` ile `√Δ` birbirine çok yakın olunca büyük iki sayının farkı alınır ve hassasiyet kaybolur. ECEF'te sayılar ~6×10⁶ olduğu için bu gerçek bir risk. Bunun yerine `q = −½(b + sign(b)·√Δ)`, `t1 = q/a`, `t2 = c/q` kullanıyoruz.
- `(a+h, b+h)` elipsoidi "her yerde tam h yükseklik" yüzeyi değil, ama birkaç km'de fark milimetre düzeyinde.

## Bulgu: düz zemin uzakta neden bozulur?
`DuzZeminUzaktaBozulur` testi: 2000 m yükseklik, tilt −5°. Hedef ~23 km ötede.
- O mesafede gerçek zemin teğet düzlemin `d²/2R ≈ 41 m` altında.
- Işın bu 41 m'yi kapatmak için `41 / tan 5° ≈ 470 m` daha ilerliyor.
- Testte ölçülen fark 490 m. Formül de bunu söylüyor. "Zemin" dediğimiz nokta aslında 41 m havada.

**Kural:** Sığ bakış ve uzun menzilde düz zemin kullanma. Yakında (birkaç km, dik bakış) fark santimetre düzeyinde (`ElipsoidYakindaDuzZeminleAyni`).

## Sorular
1. Menzil iki katına çıkınca düz zemin hatası kaç katına çıkar?
2. Atmosferik kırılma ışını hafifçe aşağı büker. Bu, düz zemin hatasını büyütür mü küçültür mü?
3. `elipsoid_kesisim`'de `qc ≤ 0` neden "başarısız" kabul ediliyor?

---
<details><summary>Cevaplar</summary>

1. Yaklaşık **4 kat** (düşme `d²` ile büyür). Sığ açıda `tanθ` da küçüldüğü için pratikte daha da fazla.
2. **Küçültür.** Kırılma ışını Dünya'nın eğriliğiyle aynı yöne büker; standart atmosferde etkisi Dünya yarıçapını ~4/3 kat büyütmek gibidir. Bu projede kırılmayı modellemiyoruz; 20 km üstü menzillerde önemli olur.
3. `qc` başlangıç noktasının yüzeye göre konumunu gösteriyor; `≤ 0` ise İHA yüzeyin içinde ya da altında. Bu, girilen yüksekliğin yanlış olduğu anlamına gelir (örneğin GPS'te elipsoid/jeoid karışıklığı).
</details>

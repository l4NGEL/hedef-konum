# 7. Doğrulama: sentetik uçuş, yanlılık ve üçgenleme

**Kod:** [senaryo.hpp](../include/hk/senaryo.hpp), [senaryo.cpp](../src/senaryo.cpp), [hk_senaryo.cpp](../app/hk_senaryo.cpp) · **Test:** [test_senaryo.cpp](../tests/test_senaryo.cpp)

## Amaç
Gerçek uçuş verimiz yok. Bu yüzden **gerçeği bildiğimiz bir dünya** kuruyoruz ve sistemi orada sınıyoruz.

1. Hedefi sentetik tepelik arazide bilinen bir noktaya koy.
2. İHA hedefin etrafında 1500 m yarıçapla, 800 m AGL'de, 25 m/s hızla daire çizsin. Yatış açısı koordineli dönüş formülünden: `tan φ = v²/(gR)` ≈ 2,4°.
3. Gimbal hedefe dönsün (`gimbali_yonelt`), ama takip mükemmel olmasın: ±40 piksellik salınım.
4. **İleri model** (`hedefi_izdusur`): hedef hangi piksele düşüyor?
5. Telemetriye ve piksele hata ekle: her karede yeniden çekilen **rastgele** hata + uçuş boyunca **sabit** kalan yanlılık.
6. **Geri model** ile hedefi bul ve gerçekle karşılaştır.

**İlk kontrol:** Hatasız senaryoda sonuç milimetre düzeyinde tutmalı (`HatasizSenaryoMilimetre`). Tutmuyorsa ileri ve geri modelden biri yanlış. Bu test, bölüm 1–5'in tamamının birlikte doğru çalıştığını gösteriyor.

## Çok bakışlı üçgenleme (`ucgenle`)
Her kare bir ışın veriyor. Farklı açılardan bakan ışınlar hedefte kesişir. Bu yöntem DEM gerektirmiyor:

`min Σ |(I − ddᵀ)(p − o)|²  →  (Σ Pᵢ) p = Σ Pᵢ oᵢ`, burada `Pᵢ = I − dᵢdᵢᵀ`

Bu 3×3'lük bir doğrusal sistem. Işınlar birbirine paralelse matris tekilleşir ve çözüm reddedilir (`UcgenlemeParalelIsinlardaReddeder`).

## Sonuçlar ([sonuclar/duyarlilik.md](../sonuclar/duyarlilik.md), [harita](../sonuclar/harita.html))

| senaryo | kare | tek kare CEP50 | kareler ortalaması | üçgenleme |
|---|---:|---:|---:|---:|
| rastgele hata, tam tur | 378 | 20,6 m | **1,4 m** | 2,1 m |
| + sabit yanlılık, tam tur | 378 | 35,1 m | 8,8 m | **2,0 m** |
| + sabit yanlılık, 40 s yay | 41 | 42,0 m | 23,9 m | 24,4 m |

Sabit yanlılık: yaw +1°, gimbal tilt +0,3°, DEM +8 m.

## Okuma
- **Rastgele hata ortalamayla söner:** 378 kare → yaklaşık √378 ≈ 19 kat iyileşme bekleriz; 20,6 m → 1,4 m.
- **Yanlılık ortalamayla sönmez, ama geometri onu söndürebilir.** Yaw yanlılığı hedefi İHA'ya göre yana kaydırır. İHA hedefin etrafında döndükçe "yan" yönü de dönüyor, bu yüzden tam turda kayma büyük ölçüde sıfırlanıyor. Düz zeminde neredeyse tamamen siliniyor (`DuzZemindeTamTurYawYanliliginiSondurur`).
- **Tepelik arazide tam sönmüyor** (`TepelikAraziYawYanliliginiTamSondurmez`): yana kayan nokta başka bir yükseklikteki zemine düşüyor ve bu da bakış yönünde ek bir kayma yaratıyor. Bu kayma arazinin şekline bağlı olduğu için tur boyunca sönmüyor. Ortalamanın 8,8 m'de kalmasının bir sebebi bu.
- **Üçgenleme DEM yanlılığından etkilenmiyor.** DEM'i hiç kullanmadığı için +8 m'lik DEM hatası ona işlemiyor: tam turda 2,0 m.
- **Kısa yay kötü:** 40 saniyede İHA sadece ~38° dönüyor. Işınlar neredeyse paralel kalıyor, yanlılık sönmüyor, üçgenleme de zayıf. Sahada bunun anlamı şu: tek geçişte (yörüngesiz) alınmış ölçümler yanlılığa açık.

## Sınırlar (bilerek)
- Arazi ve senaryo sentetik. Gerçek SITL (ArduPilot/PX4) ve gerçek video kullanılmadı.
- Hatalar Gauss ve birbirinden bağımsız varsayıldı. Gerçekte IMU hataları zamanla ilişkili (yavaş kayma) ve GPS hataları da korelasyonlu.
- Atmosferik kırılma, kamera-gimbal montaj hatası ve zaman senkronizasyonu (telemetri ile video arasındaki gecikme) modellenmedi. Zaman kayması pratikte en sinsi hata: 25 m/s hızda 100 ms gecikme 2,5 m konum hatası demek, dönüş sırasında ise açı hatası da ekliyor.

## Sorular
1. Üçgenleme her zaman ortalamadan iyi mi?
2. Hedef hareket ediyorsa üçgenleme ne olur?
3. Bir sonraki adımda ne eklerdin: Kalman filtresi mi, zaman senkronizasyonu modeli mi?

---
<details><summary>Cevaplar</summary>

1. Hayır. Rastgele hata senaryosunda ortalama 1,4 m ile daha iyi, üçgenleme 2,1 m. Üçgenleme DEM bilgisini kullanmıyor; DEM doğruysa elinde fazladan bilgi olan ortalama kazanır. DEM yanlıysa üçgenleme kazanır.
2. Bozulur, çünkü bütün ışınların aynı noktada kesiştiği varsayımı çöker. Hareketli hedefte her kareyi DEM ile ayrı ayrı çözüp bir hareket modeliyle (sabit hız Kalman filtresi) birleştirmek gerekir.
3. Gerçek sisteme bağlanacaksa önce zaman senkronizasyonu (video karesinin zaman damgası ↔ telemetri ara değeri). Kalman filtresi ise hareketli hedef ve kademeli iyileşme için ikinci adım.
</details>

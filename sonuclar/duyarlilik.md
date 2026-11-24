# Duyarlılık analizi (sentetik)

Kamera 1280x720, HFOV 30°. Hedef sentetik tepelik arazide. Her satır yalnız o hata
kaynağı açıkken 2000 örneklik Monte Carlo. Birim: metre (yatay).

## Dik bakış (AGL 800 m, yatay 300 m)

Eğik menzil 854 m, gimbal tilt -69.4°.

| kaynak | CEP50 | CEP90 | RMS | kuzey yanlılık | doğu yanlılık |
|---|---:|---:|---:|---:|---:|
| GPS yatay | 2.3 | 4.2 | 2.7 | +0.0 | -0.0 |
| GPS dusey | 0.9 | 2.3 | 1.4 | -0.0 | -0.0 |
| roll | 1.9 | 4.6 | 2.8 | +0.0 | -0.1 |
| pitch | 2.0 | 4.7 | 2.9 | +0.1 | -0.0 |
| yaw | 1.8 | 4.2 | 2.6 | -0.0 | +0.0 |
| gimbal pan | 0.4 | 0.9 | 0.5 | +0.0 | -0.0 |
| gimbal tilt | 0.9 | 2.3 | 1.4 | -0.0 | -0.0 |
| piksel | 0.8 | 1.5 | 1.0 | -0.0 | -0.0 |
| DEM | 1.1 | 2.9 | 1.7 | -0.0 | -0.0 |
| HEPSI | 5.1 | 9.6 | 6.3 | +0.0 | -0.1 |

## Eğik bakış (AGL 800 m, yatay 3000 m)

Eğik menzil 3105 m, gimbal tilt -15.0°.

| kaynak | CEP50 | CEP90 | RMS | kuzey yanlılık | doğu yanlılık |
|---|---:|---:|---:|---:|---:|
| GPS yatay | 1.8 | 3.6 | 2.3 | +0.0 | -0.0 |
| GPS dusey | 4.9 | 12.2 | 7.3 | -0.1 | -0.0 |
| roll | 1.9 | 4.7 | 2.9 | +0.0 | -0.1 |
| pitch | 14.3 | 33.2 | 20.3 | +0.6 | -0.0 |
| yaw | 18.9 | 44.0 | 27.0 | -0.1 | +0.1 |
| gimbal pan | 3.8 | 9.0 | 5.5 | +0.0 | -0.2 |
| gimbal tilt | 6.6 | 16.4 | 9.9 | -0.0 | -0.0 |
| piksel | 4.4 | 8.7 | 5.6 | -0.1 | -0.0 |
| DEM | 6.1 | 15.3 | 9.2 | -0.0 | -0.0 |
| HEPSI | 31.4 | 58.6 | 38.4 | +0.5 | -0.2 |

# Yörünge senaryoları

İHA hedef etrafında 1500 m yarıçap, 800 m AGL, 25 m/s daire çiziyor; gimbal
hedefi ±40 piksel salınımla takip ediyor. Tek kare hata dağılımı ile, karelerin
ortalaması ve çok bakışlı üçgenleme (DEM'siz) karşılaştırılıyor.

| senaryo | kare | tek kare CEP50 | tek kare CEP90 | ortalama hatası | üçgenleme hatası |
|---|---:|---:|---:|---:|---:|
| rastgele hata, tam tur | 378 | 20.6 | 46.2 | 1.4 | 2.1 |
| + sabit yanlılık, tam tur | 378 | 35.1 | 60.0 | 8.8 | 2.0 |
| + sabit yanlılık, 40 s yay | 41 | 42.0 | 80.7 | 23.9 | 24.4 |

Sabit yanlılık: yaw +1°, gimbal tilt +0,3°, DEM +8 m.

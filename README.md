# 🔑 PDF Çilingiri - Evrak & Belge Stüdyosu (v36)

> **Tamamen Çevrimdışı, Güvenli ve Hızlı PDF & Evrak Düzenleme Stüdyosu**  
> Belgeleriniz hiçbir sunucuya yüklenmez, verileriniz bilgisayarınızda güvende kalır.

---

## 📥 Doğrudan İndir (Direct Download)

GitHub Releases üzerinden tek tıkla her zaman en son güncel sürümü indirebilirsiniz:

* 🚀 **[PDF Çilingiri (Saf C++ / Qt 6 Portable .ZIP İndir)](https://github.com/mkrts88/onlinepdf/releases/latest/download/PDFCilingiri-v36-Windows-Portable.zip)** *(Önerilen - Donanım Hızlandırmalı, 0 Gecikme)*
* 📦 **[PDF Çilingiri (Tek Dosya Portable .EXE İndir)](https://github.com/mkrts88/onlinepdf/releases/latest/download/PDFCilingiri-v36-SingleFile.exe)** *(Kurulumsuz Tek Dosya)*

---

## ✨ Öne Çıkan Özellikler

### 🛡️ 1. %100 Yerel ve Gizli (Zero Server Upload)
* Belgeleriniz internete yüklenmez, üçüncü taraf sunuculara gitmez.
* KVKK, ticari sırlar, avukatlık ve mali müşavirlik evrakları için tam uyumludur.

### 📐 2. CamScanner Kalitesinde Perspektif Düzeltme (Keystone Warp)
* Telefonla veya açılı çekilmiş evrakların 4 köşesini dinamik **2.5x Büyüteçli Vizör (Loupe)** ile milimetrik olarak hizalayın.
* Tek tıkla sayfayı tam dikdörtgen A4 formatına doğrultun.

### 🧹 3. Leke Silici Fırça & Geri Al (Undo/Redo)
* Zımba deliklerini, parmak izlerini, istenmeyen gölgeleri ve lekeleri fırça yardımıyla silin.
* Yapılan tüm işlemleri `Ctrl + Z` ile anında geri alabilir veya `Ctrl + Y` ile yineleyebilirsiniz.

### 🏛️ 4. Resmi Kurum ve Faks Modu (300 DPI CCITT Group 4)
* UYAP, noter, e-Devlet ve resmi başvuru sistemlerine tam uyumlu, dosya boyutunu küçülten ultra net siyah-beyaz belge üretimi.

### ⚡ 5. Çok Çekirdekli Asenkron Mimari (Multithreaded Worker)
* `QThreadPool` arka plan işçileri sayesinde yüzlerce sayfalık PDF'lerde dahi arayüz asla donmaz (`(Yanıt Vermiyor)` uyarısı oluşmaz).
* LRU akıllı bellek önbelleği (`MemoryCache`) sayesinde RAM tüketimi her zaman minimumda kalır.

---

## ⌨️ Klavye Kısayolları

| Kısayol | İşlev |
| :--- | :--- |
| `Ctrl + O` | Belge Aç / Ekle... |
| `Ctrl + S` | Birleştir ve Kaydet... |
| `Ctrl + A` | Tüm Sayfaları Seç |
| `Ctrl + I` | Sayfa Seçimini Tersine Çevir |
| `Delete` | Seçilen Sayfaları Kaldır |
| `Space` | Odaktaki Kartın Seçimini Değiştir |
| `F2` / `Enter` | Odaktaki Sayfayı İnce Ayar Modunda Aç |
| `R` | Odaktaki Sayfayı 90° Döndür |
| `Ctrl + Z` | Son İşlemi Geri Al |
| `Ctrl + Y` | Geri Alınan İşlemi Yinele |

---

## 🛠️ Kaynak Koddan Derleme (C++ / Qt 6)

### Gereksinimler:
* GCC 15+ (MinGW64)
* Qt 6.9+ (Widgets, Gui, Core)
* Poppler-Qt6
* CMake 3.20+ ve Ninja

```bash
cd native_qt
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

---

## 📄 Lisans
Bu proje [MIT Lisansı](LICENSE) altında korunmaktadır.
Tasarım & Geliştirme: **Vezir**

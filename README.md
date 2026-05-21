# Çok Threadli Dosya Kopyalama ve Senkronizasyon Aracı

C ile yazılmış, POSIX threads (pthreads) tabanlı, kaynak bir dizini hedef dizine senkronize eden komut satırı aracı. Bir scanner thread'i kaynak dizini özyinelemeli olarak tarar; eksik veya değişmiş dosyaları thread-safe bir kuyruğa atar; birden çok worker thread kuyruktan iş çekerek dosyaları paralel olarak kopyalar.

## Derleme

```bash
make
```

Bağımlılıklar: `gcc`, `make`, POSIX threads (Linux / Unix).

## Kullanım

```bash
./copy_tool <thread_sayisi> <kaynak_dizin> <hedef_dizin>
```

Örnek:

```bash
./copy_tool 4 ./source_dir ./dest_dir
```

Tüm işlemler, çalıştırdığın dizindeki `copy_tool.log` dosyasına timestamp ve thread ID ile birlikte yazılır.

## Mimari

Üretici-tüketici (producer-consumer) deseni üzerine kurulmuştur.

| Modül       | Görev                                                              |
|-------------|-------------------------------------------------------------------|
| `main.c`    | Argüman parse, thread oluşturma, kuyruğun yaşam döngüsü           |
| `scanner.c` | Kaynak dizini recursive tarar, iş öğelerini kuyruğa ekler (producer) |
| `queue.c`   | Mutex + condition variable ile korunmuş, thread-safe FIFO kuyruk  |
| `worker.c`  | Kuyruktan iş alır, dosyayı blok blok kopyalar (consumer)          |
| `log.c`     | Thread-safe loglama (mutex korumalı)                              |

Senkronizasyon: `pthread_mutex_t` ile karşılıklı dışlama, `pthread_cond_t` (`not_empty`, `not_full`) ile bekleme/uyandırma.

## Özellikler

- Recursive dizin tarama (alt dizinler dahil)
- Hedefte eksik olan dosyaların otomatik kopyalanması
- Modification time (`mtime`) karşılaştırması ile değişmiş dosyaların güncellenmesi
- Büyük dosyalar için blok bazlı okuma/yazma (4 KB blok)
- Tüm işlemler için detaylı log (`copy_tool.log`)
- Yapılandırılabilir worker thread sayısı

## Log Formatı

```
[2025-05-16 14:23:01] [tid:140234567892480] [COPY]   ./kaynak/a.txt -> ./hedef/a.txt (1024 bayt)
[2025-05-16 14:23:01] [tid:140234567892481] [UPDATE] ./kaynak/b.log yenilendi (2048 bayt)
[2025-05-16 14:23:01] [tid:140234567892482] [ERROR]  Kaynak dosya acilamadi: ./kaynak/locked.bin
```

## Performans Karşılaştırması

`benchmark.sh` scripti, tek thread ile çoklu thread senaryolarını otomatik olarak kıyaslar. Her senaryo için taze test verisi üretilir; güvenilir ölçüm için `sudo` ile çalıştırılması önerilir (OS page cache'i temizler):

```bash
chmod +x benchmark.sh
sudo ./benchmark.sh
```

Gerçek ölçüm sonuçları (300 dosya, ~285 MB, SSD, temiz cache, iki çalıştırmanın ortalaması):

| Thread sayısı | Süre (s) | Hızlanma |
|--------------:|---------:|---------:|
| 1             |    1.442 |    1.00x |
| 2             |    0.962 |    1.50x |
| 4             |    0.774 |    1.86x |
| 8             |    0.805 |    1.79x |

**Gözlemler:**

- Thread sayısı arttıkça kopyalama süresi belirgin şekilde düşer; 4 thread en verimli noktadır (1.86x hızlanma).
- 8 thread'de hafif yavaşlama gözlemlenir: I/O-bound iş yükünde disk bant genişliği dolar,
  ek thread yalnızca mutex contention ve context-switch overhead getirir.
- Bu davranış I/O-bound iş yüklerinde beklenen ve literatürde bilinen bir örüntüdür.
- Sonuçlar diske (HDD/SSD), dosya boyut dağılımına ve CPU çekirdek sayısına göre değişir.

## Proje Yapısı

```
.
├── Makefile
├── README.md
├── benchmark.sh
├── include/
│   ├── common.h
│   ├── queue.h
│   ├── scanner.h
│   ├── worker.h
│   └── log.h
├── src/
│   ├── main.c
│   ├── queue.c
│   ├── scanner.c
│   ├── worker.c
│   └── log.c
├── source_dir/         # Test source directory
└── dest_dir/           # Test destination directory
```

## Temizleme

```bash
make clean
```

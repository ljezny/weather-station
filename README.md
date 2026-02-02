# Weather Station - Radarová mapa srážek

Aplikace pro ESP32 s e-ink displejem, která zobrazuje aktuální radarovou mapu srážek nad Českou republikou.

## Hardware

- **Deska**: FireBeetle ESP32 (nebo kompatibilní ESP32)
- **Displej**: 7.5" e-ink displej 800x480 (3 barvy: černá, bílá, červená)
- **Napájení**: LiPo baterie (volitelně)

## Funkce

- Stahování aktuálních radarových dat z RainViewer API
- Zobrazení mapy srážek nad ČR
- Legenda intenzity srážek
- Zobrazení času aktualizace radaru
- Stav baterie a WiFi signálu
- Deep sleep pro úsporu energie (výchozí interval 10 minut)

## Konfigurace

Před kompilací upravte WiFi přihlašovací údaje v souboru `src/weather_station.cpp`:

```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

## Kompilace a nahrání

```bash
# Kompilace
pio run

# Nahrání do ESP32
pio run --target upload

# Sledování sériového výstupu
pio device monitor
```

## Struktura projektu

```
weather-station/
├── platformio.ini          # PlatformIO konfigurace
└── src/
    ├── weather_station.cpp # Hlavní program
    └── weather/
        ├── API/
        │   ├── RadarAPI.hpp    # Stahování radarových dat
        │   └── PngDecoder.hpp  # Dekódování PNG obrázků
        ├── UI/
        │   ├── Display.cpp/.hpp # Ovládání e-ink displeje
        │   ├── Screen.cpp/.hpp  # Kreslení obrazovek
        │   └── *.h              # Fonty a grafické utility
        ├── Utils/
        │   ├── Battery.hpp      # Měření napětí baterie
        │   ├── TimeSync.hpp     # Synchronizace času
        │   └── WiFiSignal.hpp   # Síla WiFi signálu
        ├── Storage/
        │   └── WeatherConfiguration.hpp # Ukládání konfigurace
        └── Logging/
            └── Logging.hpp      # Debug výpisy
```

## API

Aplikace využívá bezplatné API [RainViewer](https://www.rainviewer.com/api.html) pro získání radarových dat.

### Tile URL formát
```
https://tilecache.rainviewer.com{path}/256/{zoom}/{x}/{y}/2/1_1.png
```

Kde:
- `{path}` - cesta získaná z API
- `{zoom}` - úroveň přiblížení (6 pro celou ČR)
- `{x}`, `{y}` - souřadnice dlaždice

## Přizpůsobení

### Interval aktualizace

Změňte hodnotu `refreshIntervalMinutes` v `WeatherConfiguration`:

```cpp
refreshIntervalMinutes = 5;  // Aktualizace každých 5 minut
```

### Časové pásmo

Výchozí nastavení je pro Českou republiku (CET/CEST). Změňte v `WeatherConfiguration`:

```cpp
timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
```

## Licence

Projekt je inspirován projektem calendar-station od stejného autora.

## Poznámky

- E-ink displej vyžaduje specifické piny SPI:
  - CS: GPIO 5
  - DC: GPIO 17
  - RST: GPIO 16
  - BUSY: GPIO 4
  - PWR_EN: GPIO 2

- Pro prodloužení výdrže baterie je využíván deep sleep mezi aktualizacemi

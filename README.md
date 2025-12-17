# QR Code Presenter (ESP32 + ePaper + Keypad)

Cíl: zadáš částku na klávesnici, na e-ink se průběžně zobrazuje, potvrzením se zobrazí QR platba (SPD / „QR Platba“).

## Displej (doporučení)
Pro začátek je nejsnazší koupit ePaper jako **modul s PCB a pinheaderem** (typicky Waveshare). 
Good Display `GDEY0154D67` je OK, ale pokud kupuješ jen "holý" panel s FPC kabelem, budeš potřebovat ještě **driver/breakout board**.

Aktuálně je v projektu přednastavený typ displeje:
- `GDEY0154D67` (1.54\" 200x200, B/W) přes knihovnu GxEPD2

## Zapojení (SPI)
Používají se piny z `include/pins.h`:
- EPD: `CS`, `DC`, `RST`, `BUSY` + SPI `SCK/MOSI`
- Keypad: 4 řádky + 4 sloupce

## Konfigurace
V LittleFS je soubor `/config.ini` (viz `data/config.ini`).

## Instalace PlatformIO (CLI)
Na Ubuntu typicky:

```bash
python3 -m pip install --user -U platformio
```

(Alternativně `pipx install platformio`.)

## Build + upload
V kořeni projektu:

```bash
pio run
```

Nahrání LittleFS (config.ini):

```bash
pio run -t uploadfs
```

Nahrání firmware:

```bash
pio run -t upload
```

Serial monitor:

```bash
pio device monitor
```

## Ovládání
- čísla: zadávání částky (v haléřích) 
- `*`: backspace
- `C`: vymazat
- `#`: potvrdit → zobrazit QR
- `D`: zpět na zadávání

Pozn.: částka se zadává jako "haléře" (např. zadáš `12345` => 123.45). Pokud chceš UX jako kalkulačka (automatická desetinná čárka), můžu to upravit.

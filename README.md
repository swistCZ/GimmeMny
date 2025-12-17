# GimmeMny

Přenosné zařízení s e-paper displejem pro generování platebních QR kódů standardu SPAYD. Funguje jako jednoduchá kalkulačka a následně zobrazí QR kód pro výslednou částku.

![Device Placeholder Image](https://via.placeholder.com/300x300.png?text=GimmeMny+Device)

## Klíčové vlastnosti

- **Generování SPAYD QR kódů**: Vytváří platné QR kódy pro bezhotovostní platby.
- **Funkce kalkulačky**: Sčítá více položek (`A` pro `+`, `B` pro `=`) před zobrazením finálního QR kódu.
- **E-Paper Displej**: Energeticky úsporný displej s výbornou čitelností. Obrazovka se při zobrazení QR kódu otočí o 180° pro snadné ukázání zákazníkovi.
- **Konfigurace přes soubor**: Bankovní účet a další parametry se snadno nastavují v souboru `config.ini`.
- **Úsporný režim (Deep Sleep)**: Zařízení se po určité době nečinnosti automaticky uspí, aby šetřilo baterii. Probuzení proběhne stiskem jakékoliv klávesy.
- **Indikátor stavu baterie**: V rohu displeje se zobrazuje aktuální stav nabití baterie.

## Hardware

- **Řídicí deska**: `ESP32 Dev Kit` (nebo kompatibilní)
- **Displej**: `GDEY0154D67` (1.54" 200x200, černobílý e-paper)
- **Klávesnice**: Membránová maticová klávesnice `4x4`
- **Napájení**: LiPo baterie

### Zapojení klávesnice

Klávesnice 4x4 má 8 pinů. Pro správnou funkci je nutné ji propojit s ESP32 podle následující tabulky. Na klávesnici z LaskaKitu (viz. nákres níže) jsou piny obvykle očíslovány 1-8. Předpokládáme, že piny 1-4 odpovídají řádkům (R1-R4) a piny 5-8 odpovídají sloupcům (C1-C4), jak je znázorněno na schématu.

**Zapojení pinů klávesnice k ESP32:**

| Klávesnice (Pin na modulu) | Klávesnice (Funkce) | ESP32 GPIO |
| :------------------------- | :------------------ | :--------- |
| Pin 1                      | ROW 0 (R1)          | GPIO 32    |
| Pin 2                      | ROW 1 (R2)          | GPIO 33    |
| Pin 3                      | ROW 2 (R3)          | GPIO 25    |
| Pin 4                      | ROW 3 (R4)          | GPIO 26    |
| Pin 5                      | COL 0 (C1)          | GPIO 27    |
| Pin 6                      | COL 1 (C2)          | GPIO 14    |
| Pin 7                      | COL 2 (C3)          | GPIO 12    |
| Pin 8                      | COL 3 (C4)          | GPIO 13    |

![4x4 Keypad Wiring Diagram](https://cdn.myshoptet.com/usr/www.laskakit.cz/user/documents/upload/4x4%20keyboard.bmp)

**Důležité:**
*   Vždy zkontrolujte, zda se piny na vaší konkrétní klávesnici shodují s předpokládaným pořadím.
*   Piny řádků (ROW 0-3) jsou nakonfigurovány pro probuzení ESP32 z režimu Deep Sleep.

## Konfigurace

Veškeré nastavení se provádí v souboru `data/config.ini`. Po úpravě je nutné nahrát souborový systém do zařízení.

**Příklad `config.ini`:**
```ini
[payment]
# IBAN účtu příjemce
acc_iban=CZ5855000000001265098001
# Měna (CZK)
cc=CZK
# Zpráva pro příjemce
msg=DEKUJI ZA NAKUP

[ui]
# Text zobrazený na úvodní obrazovce
title=GimmeMny
# Časovač pro uspání v sekundách (0 = nikdy)
sleep_timeout_s=300
```

Pro nahrání souboru použijte příkaz:
```bash
pio run -t uploadfs
```

## Instalace a nahrání firmware

Projekt je postaven na [PlatformIO](https://platformio.org/).

1.  **Sestavení projektu:**
    ```bash
    pio run
    ```
2.  **Nahrání firmware do zařízení:**
    ```bash
    pio run -t upload
    ```
3.  **Sledování sériové komunikace:**
    ```bash
    pio device monitor
    ```

## Ovládání

| Klávesa     | Režim zadávání             | Režim zobrazení QR          |
| :---------- | :------------------------- | :-------------------------- |
| **`0-9`**   | Zadávání částky (v haléřích) | -                           |
| **`*`**     | Smazání posledního znaku   | -                           |
| **`C`**     | Reset / Vynulování         | -                           |
| **`A`**     | Přičtení k mezisoučtu (`+`)  | -                           |
| **`B`**     | Zobrazení celkového součtu (`=`) | -                           |
| **`#`**     | Generovat a zobrazit QR kód  | -                           |
| **`D`**     | -                          | Zpět na zadávání           |
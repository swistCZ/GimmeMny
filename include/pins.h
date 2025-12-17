#pragma once

// ----------------------
// ePaper (SPI) piny
// ----------------------
// Pozn.: Konkrétní SPI piny (SCK/MOSI) bere knihovna z ESP32 HW SPI.
// Pokud používáš VSPI default: SCK=18, MOSI=23.

// Tyhle 4 piny si nejspíš budeš upravovat podle PCB:
static constexpr int PIN_EPD_CS   = 5;
static constexpr int PIN_EPD_DC   = 17;
static constexpr int PIN_EPD_RST  = 16;
static constexpr int PIN_EPD_BUSY = 4;

// ----------------------
// 4x4 matice klávesnice
// ----------------------
// Výchozí mapování kláves:
// 1 2 3 A
// 4 5 6 B
// 7 8 9 C
// * 0 # D
//
// Doporučené akce:
// - čísla: zadávání
// - '*': backspace
// - 'C': clear
// - '#': potvrdit a zobrazit QR
// - 'D': zpět na zadávání

static constexpr int KEYPAD_ROWS = 4;
static constexpr int KEYPAD_COLS = 4;

// Uprav podle reálného zapojení (GPIO, které nejsou problematické při bootu)
static constexpr int PIN_KEYPAD_ROW_0 = 32;
static constexpr int PIN_KEYPAD_ROW_1 = 33;
static constexpr int PIN_KEYPAD_ROW_2 = 25;
static constexpr int PIN_KEYPAD_ROW_3 = 26;

static constexpr int PIN_KEYPAD_COL_0 = 27;
static constexpr int PIN_KEYPAD_COL_1 = 14;
static constexpr int PIN_KEYPAD_COL_2 = 12;
static constexpr int PIN_KEYPAD_COL_3 = 13;

// ----------------------
// Měření napětí baterie
// ----------------------
// DŮLEŽITÉ: Zkontroluj verzi desky LaskaKit ESPink!
// v3.5 používá GPIO9
// v2.7 používá GPIO34
// Pokud používáš jinou verzi, uprav tento pin.
static constexpr int PIN_BATTERY_VOLTAGE = 9; // Předpokládáme v3.5

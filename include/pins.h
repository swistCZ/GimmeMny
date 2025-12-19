#pragma once

// ----------------------
// ePaper (SPI) piny
// ----------------------
// LaskaKit ESPink v3.5 pinout
// HW SPI pro ESP32-S3: MOSI=11, SCK=12
static constexpr int PIN_EPD_CS   = 10;
static constexpr int PIN_EPD_DC   = 48;
static constexpr int PIN_EPD_RST  = 45;
static constexpr int PIN_EPD_BUSY = 38;

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

// LaskaKit ESPink v3.5 - piny upravené podle dostupnosti
static constexpr int PIN_KEYPAD_ROW_0 = 4;
static constexpr int PIN_KEYPAD_ROW_1 = 5;
static constexpr int PIN_KEYPAD_ROW_2 = 6;
static constexpr int PIN_KEYPAD_ROW_3 = 7;

static constexpr int PIN_KEYPAD_COL_0 = 15;
static constexpr int PIN_KEYPAD_COL_1 = 16;
static constexpr int PIN_KEYPAD_COL_2 = 17;
static constexpr int PIN_KEYPAD_COL_3 = 19;

// ----------------------
// Měření napětí baterie
// ----------------------
// DŮLEŽITÉ: Zkontroluj verzi desky LaskaKit ESPink!
// v3.5 používá GPIO9
// v2.7 používá GPIO34
// Pokud používáš jinou verzi, uprav tento pin.
static constexpr int PIN_BATTERY_VOLTAGE = 9; // Předpokládáme v3.5

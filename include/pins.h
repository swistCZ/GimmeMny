#pragma once

// ----------------------
// ePaper (SPI) piny
// ----------------------
// LaskaKit ESPink v3.5 pinout
// HW SPI pro ESP32-S3:
static constexpr int PIN_EPD_SCK  = 12;
static constexpr int PIN_EPD_MOSI = 11;
static constexpr int PIN_EPD_CS   = 10;
static constexpr int PIN_EPD_DC   = 48;
static constexpr int PIN_EPD_RST  = 45;
static constexpr int PIN_EPD_BUSY = 38;
// Pin pro zapnutí napájení displeje
static constexpr int PIN_EPD_POWER = 47;

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

// LaskaKit ESPink v3.5 - piny upravené podle funkčního zapojení
// Pořadí řádků odpovídá fyzickému zapojení (shora dolů)
static constexpr int PIN_KEYPAD_ROW_0 = 19; // Ribbon Pin 8
static constexpr int PIN_KEYPAD_ROW_1 = 17; // Ribbon Pin 7
static constexpr int PIN_KEYPAD_ROW_2 = 16; // Ribbon Pin 6
static constexpr int PIN_KEYPAD_ROW_3 = 15; // Ribbon Pin 5

// Pořadí sloupců je reverzní, aby odpovídalo matici `keys` v main.cpp
static constexpr int PIN_KEYPAD_COL_0 = 7;  // Ribbon Pin 4
static constexpr int PIN_KEYPAD_COL_1 = 6;  // Ribbon Pin 3
static constexpr int PIN_KEYPAD_COL_2 = 5;  // Ribbon Pin 2
static constexpr int PIN_KEYPAD_COL_3 = 4;  // Ribbon Pin 1

// ----------------------
// Měření napětí baterie
// ----------------------
// DŮLEŽITÉ: Zkontroluj verzi desky LaskaKit ESPink!
// v3.5 používá GPIO9
// v2.7 používá GPIO34
// Pokud používáš jinou verzi, uprav tento pin.
static constexpr int PIN_BATTERY_VOLTAGE = 9; // Předpokládáme v3.5

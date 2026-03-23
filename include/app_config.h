#pragma once

#include <Arduino.h>

struct PaymentConfig {
  // Povinné pro QR platbu
  String acc_iban;   // např. CZ5855000000001265098001
  String cc;         // CZK

  // Volitelné
  String bic;        // např. RZBCCZPP (když chceš)
  String msg;        // zpráva pro příjemce
  String rn;         // jméno příjemce
  String x_vs;       // variabilní symbol
  String x_ss;       // specifický symbol
  String x_ks;       // konstantní symbol
};

struct UiConfig {
  String title;      // text nahoře
  bool translit;     // převod diakritiky v MSG/RN do ASCII
  int sleep_timeout_s; // časovač pro deep sleep v sekundách
  String goodbye_line1; // První řádek textu před deep sleep
  String goodbye_line2; // Druhý řádek textu před deep sleep
};

struct PowerConfig {
  bool display_charge_status_enabled; // Zobrazovat stavy nabíjení na displeji
};

struct AppConfig {
  PaymentConfig pay;
  UiConfig ui;
  PowerConfig power;
};

AppConfig defaultConfig();

// Načte /config.ini z LittleFS. Když soubor neexistuje, vrátí false.
bool loadConfigFromFs(AppConfig& cfg);

// Zapíše /config.ini do LittleFS.
bool saveConfigToFs(const AppConfig& cfg);

// Pomocná funkce pro výpis configu do Serial (debug).
void printConfig(const AppConfig& cfg, Stream& out);

// --- UI texty a konstanty ---
// Centralizujeme texty pro snazší úpravu a budoucí překlady
namespace UI_STRINGS {
  constexpr const char* HELP_TEXT_ENTER_AMOUNT = "A=+ B== C=RST *=DEL #=QR";
  constexpr const char* HELP_TEXT_QR = "D=ZPET";
  constexpr const char* DEFAULT_TITLE = "GimmeMny";
  constexpr const char* RUNNING_TOTAL_PREFIX = "Suma: ";
  constexpr const char* BATTERY_PERCENT_SUFFIX = "%";
  constexpr const char* DEFAULT_CURRENCY = "CZK";

  constexpr const char* CHARGING_TEXT = "Nabijim...";
  constexpr const char* CHARGED_TEXT = "Baterie OK";
  constexpr const char* WARNING_NOT_CHARGING_TEXT = "VAROVANI: Nenabijim!";
}


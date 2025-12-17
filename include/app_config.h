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
};

struct AppConfig {
  PaymentConfig pay;
  UiConfig ui;
};

AppConfig defaultConfig();

// Načte /config.ini z LittleFS. Když soubor neexistuje, vrátí false.
bool loadConfigFromFs(AppConfig& cfg);

// Zapíše /config.ini do LittleFS.
bool saveConfigToFs(const AppConfig& cfg);

// Pomocná funkce pro výpis configu do Serial (debug).
void printConfig(const AppConfig& cfg, Stream& out);

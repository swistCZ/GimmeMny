#include <Arduino.h>
#include <SPI.h>
#include <LittleFS.h>
#include <esp_sleep.h> // pro deep sleep

#include <Keypad.h>
#include <qrcode.h>

#include "pins.h"
#include "app_config.h"
#include "spayd.h"

#if APP_DISPLAY_GDEY0154D67
  #include <GxEPD2_BW.h>
  // GDEY0154D67 (1.54" 200x200) - SSD1681

  using DisplayDriver = GxEPD2_BW<GxEPD2_154_GDEY0154D67, GxEPD2_154_GDEY0154D67::HEIGHT>;
  DisplayDriver display(GxEPD2_154_GDEY0154D67(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
#else
  #error "Zatím je podporovaný jen APP_DISPLAY_GDEY0154D67=1 (můžu doplnit další podle tvého displeje)."
#endif

static AppConfig g_cfg;

// ---------- Keypad ----------
static char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

static byte rowPins[KEYPAD_ROWS] = { PIN_KEYPAD_ROW_0, PIN_KEYPAD_ROW_1, PIN_KEYPAD_ROW_2, PIN_KEYPAD_ROW_3 };
static byte colPins[KEYPAD_COLS] = { PIN_KEYPAD_COL_0, PIN_KEYPAD_COL_1, PIN_KEYPAD_COL_2, PIN_KEYPAD_COL_3 };

static Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

// ---------- Amount input ----------
// Udržujeme částku jako řetězec jen s čísly (bez tečky), a odděleně počet desetinných míst.
// Pro CZK typicky 0 nebo 2. Pro jednoduchost budeme používat vždy 2 desetinná místa (haléře).
static String amountDigits; // např. "12345" => 123.45

// stav kalkulačky
static int64_t running_total_cents = 0;
static bool is_showing_result = false;

static int64_t amountToCents() {
  if (amountDigits.length() == 0) return 0;
  // omezíme délku aby se to vešlo do int64_t a UI
  if (amountDigits.length() > 12) return 0;
  int64_t v = 0;
  for (size_t i = 0; i < amountDigits.length(); i++) {
    char c = amountDigits[i];
    if (c < '0' || c > '9') continue;
    v = v * 10 + (c - '0');
  }
  // Změna: zadané číslo jsou koruny, převedeme na haléře
  return v * 100;
}

static String formatAmountForUi() {
  return formatAmountCz(amountToCents());
}

// ---------- Battery voltage ----------
// Konstanta z datasheetu ESP32 a LaskaKit ESPink
static constexpr int BATTERY_FULL_MV = 4200; // 4.2V
static constexpr int BATTERY_EMPTY_MV = 3200; // 3.2V (min. provozní napětí LiPo)
static constexpr int ADC_READ_COUNT = 5; // Kolikrát přečíst ADC pro zprůměrování
static constexpr float ADC_MAX_VALUE = 4095.0; // 12-bit ADC
static constexpr float ADC_REFERENCE_MV = 3300.0; // Vnitřní reference ESP32 ADC
static constexpr float VOLTAGE_DIVIDER_COEFFICIENT = 1.769388; // Z LaskaKit dokumentace

static int readBatteryMv() {
  int raw_adc = 0;
  for (int i = 0; i < ADC_READ_COUNT; i++) {
    raw_adc += analogRead(PIN_BATTERY_VOLTAGE);
    delay(1);
  }
  raw_adc /= ADC_READ_COUNT;

  // Přepočet na skutečné napětí baterie
  float measured_voltage = (raw_adc / ADC_MAX_VALUE) * ADC_REFERENCE_MV;
  float battery_voltage = measured_voltage * VOLTAGE_DIVIDER_COEFFICIENT;
  return static_cast<int>(battery_voltage);
}

static int getBatteryPercentage() {
  int mv = readBatteryMv();
  // Mapování napětí na procenta
  int percentage = map(mv, BATTERY_EMPTY_MV, BATTERY_FULL_MV, 0, 100);
  return constrain(percentage, 0, 100);
}

// ---------- UI rendering ----------
static void renderEnterScreen() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.setTextColor(GxEPD_BLACK);

    // --- Úprava pozic textu ---
    // Zobrazíme mezisoučet nahoře, nebo titul, vycentrováno
    String topText;
    int16_t x1, y1;
    uint16_t w, h;

    display.setTextSize(2);
    if (running_total_cents > 0) {
      topText = "Suma: " + formatAmountCz(running_total_cents);
    } else {
      if (g_cfg.ui.title == "GimmeMny") { // Pokud je defaultní název, nahradíme ho
        topText = "Rzacze";
      } else {
        topText = g_cfg.ui.title;
      }
    }
    display.getTextBounds(topText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, 5 + h); // Vycentrováno, 5px od horního okraje
    display.print(topText);

    // Zobrazení stavu baterie v pravém horním rohu (mělo by fungovat)
    display.setTextSize(1);
    display.setCursor(display.width() - 40, 5);
    display.print(String(getBatteryPercentage()) + "%");

    // Hlavní částka - vycentrováno
    display.setTextSize(3);
    String amountStr = formatAmountForUi() + " " + (g_cfg.pay.cc.length() ? g_cfg.pay.cc : "CZK");
    display.getTextBounds(amountStr, 0, 0, &x1, &y1, &w, &h);
    // Vertikálně centrováno uprostřed dostupné plochy (pod topText, nad helpText)
    display.setCursor((display.width() - w) / 2, display.height() / 2 + h / 2);
    display.print(amountStr);

    // Nápověda kláves - dole, vycentrováno
    display.setTextSize(1);
    String helpText = "A=+ B== C=RST *=DEL #=QR";
    display.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, display.height() - 5 - h); // Vycentrováno, 5px od spodního okraje
    display.print(helpText);
    // --- Konec úpravy pozic textu ---

  } while (display.nextPage());
}

static void renderQrScreen(const String& spayd, const String& amountUi) {
  // QR generation
  QRCode qrcode;

  // Auto-volba verze: zkusíme postupně vyšší verze, dokud se nevejdeme.
  // ECC: medium
  uint8_t version = 3;
  uint8_t qrcodeData[4000]; // dost velké pro menší verze (pro QR platbu obvykle stačí)

  bool ok = false;
  for (version = 3; version <= 15; version++) {
    int needed = qrcode_getBufferSize(version);
    if (needed > (int)sizeof(qrcodeData)) break;

    qrcode_initText(&qrcode, qrcodeData, version, ECC_MEDIUM, spayd.c_str());
    // když init proběhne, bereme to jako ok
    ok = true;
    break;
  }

  display.setFullWindow();
  display.firstPage();
        do {
          display.fillScreen(GxEPD_WHITE);
          display.setTextColor(GxEPD_BLACK);
      
          // --- Nová logika vykreslování s 10px okraji ---
          const int top_margin_y = 10;    // Prostor pro horní řádek textu (částka, baterie)
          const int bottom_margin_y = 10; // Prostor pro spodní řádek textu (D=ZPET)
          const int available_space_for_qr = display.height() - top_margin_y - bottom_margin_y;
      
          // Vypočítáme velikost modulu QR kódu
          int module_size = available_space_for_qr / qrcode.size;
          if (module_size < 1) module_size = 1;
      
          int qr_display_width = qrcode.size * module_size;
          int x_offset = (display.width() - qr_display_width) / 2;
          int y_offset_qr = top_margin_y + (available_space_for_qr - qr_display_width) / 2;
      
          // Vykreslení QR kódu
          for (int y = 0; y < qrcode.size; y++) {
            for (int x = 0; x < qrcode.size; x++) {
              if (qrcode_getModule(&qrcode, x, y)) {
                display.fillRect(x_offset + x * module_size, y_offset_qr + y * module_size, module_size, module_size, GxEPD_BLACK);
              }
            }
          }
      
          // --- Pozice textu ---
          int16_t x1, y1;
          uint16_t w, h; // height of current text
      
          // Částka pro QR - umístíme vlevo nahoře, vertikálně vycentrováno v top_margin_y
          display.setTextSize(1);
          String amountCurrency = amountUi + " " + (g_cfg.pay.cc.length() ? g_cfg.pay.cc : "CZK");
          display.getTextBounds(amountCurrency, 0, 0, &x1, &y1, &w, &h);
    uint16_t y_pos_top_text = (top_margin_y - h) / 2; // Vertikálně vycentrovat v rámci horního okraje
    display.setCursor((display.width() - w) / 2, y_pos_top_text); // Horizontálně vycentrovat
    display.print(amountCurrency);
      
          // Zobrazení stavu baterie vpravo nahoře, vertikálně vycentrováno v top_margin_y
          display.setTextSize(1);
          // Reuse 'h' for height, or recalculate if font changes
          display.setCursor(display.width() - 40, y_pos_top_text); // Stejná Y pozice jako částka
          display.print(String(getBatteryPercentage()) + "%");
      
          // Nápověda kláves (D=ZPET) - vycentrováno dole, vertikálně vycentrováno v bottom_margin_y
          display.setTextSize(1);
          String backText = "D=ZPET";
          display.getTextBounds(backText, 0, 0, &x1, &y1, &w, &h);
          uint16_t y_pos_bottom_text = display.height() - bottom_margin_y + (bottom_margin_y - h) / 2; // Y pozice pro spodní text
          display.setCursor((display.width() - w) / 2, y_pos_bottom_text);
          display.print(backText);
          // --- Konec nové logiky ---
      
        } while (display.nextPage());}

enum class UiState {
  EnterAmount,
  ShowQr,
  PreparingForDeepSleep, // Nový stav pro čekání před usnutím
};

static UiState g_state = UiState::EnterAmount;
static String g_lastSpayd;

// ---------- Deep Sleep ----------
static unsigned long last_key_press_time = 0;
static unsigned long deep_sleep_countdown_start_time = 0;
static const unsigned long DEEP_SLEEP_CLEAN_DURATION = 15000; // 15 sekund

static void goEnter() {
  display.setRotation(1);
  g_state = UiState::EnterAmount;
  amountDigits = "";
  running_total_cents = 0;
  is_showing_result = false;
  renderEnterScreen();
}

static void goQr() {
  display.setRotation(3); // otočíme o 270° (o 180° víc než základní rotace)
  int64_t cents = amountToCents();
  String spayd = buildSpayd(g_cfg, cents);
  g_lastSpayd = spayd;
  g_state = UiState::ShowQr;
  renderQrScreen(spayd, formatAmountForUi());

  Serial.println("SPAYD:");
  Serial.println(spayd);
}

static void ensureConfigFile() {
  if (LittleFS.exists("/config.ini")) return;

  // když config není, uložíme default (a uživatel může přepsat přes uploadfs nebo edit)
  AppConfig def = defaultConfig();
  saveConfigToFs(def);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // --- FIX: Přidána inicializace pro LaskaKit ESPink ---
  // 1. Zapnutí napájení displeje
  pinMode(PIN_EPD_POWER, OUTPUT);
  digitalWrite(PIN_EPD_POWER, HIGH);
  delay(100); // krátká pauza pro stabilizaci napájení

  // 2. Explicitní nastavení SPI pinů
  // MISO není pro displej potřeba (write-only), CS pin řídí knihovna GxEPD2
  SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, -1);
  // --- KONEC FIXu ---

  analogReadResolution(12); // Ujistíme se, že je 12-bit

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS init failed");
  }

  ensureConfigFile();

  g_cfg = defaultConfig();
  if (loadConfigFromFs(g_cfg)) {
    Serial.println("Config loaded from /config.ini");
  } else {
    Serial.println("Using default config");
  }
  printConfig(g_cfg, Serial);

  display.init(115200);
  display.setRotation(0);

  // Deep sleep setup
  // Konfigurujeme piny řádků klávesnice jako zdroje probuzení pro EXT1
  // Piny musí být specifikovány jako bitová maska.
  // Předpokládáme, že řádkové piny jsou při stisku klávesy HIGH.
  uint64_t wakeup_pin_mask = (1ULL << PIN_KEYPAD_ROW_0) |
                             (1ULL << PIN_KEYPAD_ROW_1) |
                             (1ULL << PIN_KEYPAD_ROW_2) |
                             (1ULL << PIN_KEYPAD_ROW_3);
  esp_sleep_enable_ext1_wakeup(wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  
  // Nastavíme počáteční čas pro kontrolu spánku
  last_key_press_time = millis();

  goEnter();
}

static void onKeyEnter(char k) {
  if (k >= '0' && k <= '9') {
    // Pokud jsme právě zobrazili výsledek, začneme zadávat nové číslo
    if (is_showing_result) {
      amountDigits = "";
      is_showing_result = false;
    }
    // max 12 číslic haléřů => do UI se vejde, a je to bezpečné
    if (amountDigits.length() < 12) {
      amountDigits += k;
    }
    renderEnterScreen();
    return;
  }

  if (k == '*') {
    // backspace - nelze mazat z výsledku
    if (is_showing_result) return;
    if (amountDigits.length() > 0) {
      amountDigits.remove(amountDigits.length() - 1);
    }
    renderEnterScreen();
    return;
  }

  if (k == 'C') { // Reset
    goEnter(); // resetuje vše do původního stavu
    return;
  }

  if (k == 'A') { // + (plus)
    running_total_cents += amountToCents();
    amountDigits = "";
    is_showing_result = false;
    renderEnterScreen();
    return;
  }

  if (k == 'B') { // = (rovná se)
    running_total_cents += amountToCents();
    // Převedeme celkový výsledek na string, ale jako koruny, ne haléře
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)(running_total_cents / 100));
    amountDigits = buf;
    
    // vynulujeme mezisoučet a označíme, že zobrazujeme konečný výsledek
    running_total_cents = 0;
    is_showing_result = true;
    renderEnterScreen();
    return;
  }

  if (k == '#') {
    // Pokud je na displeji výsledek, použijeme ten. Jinak použijeme právě zadávané číslo.
    // Není potřeba nic dělat, `amountToCents()` vždy vezme co je v `amountDigits`.
    goQr();
    return;
  }
}

static void onKeyQr(char k) {
  if (k == 'D') {
    goEnter();
    return;
  }
}

static void startDeepSleepCountdown() {
  Serial.println("Starting deep sleep countdown...");
  
  // 1. Cyklus plného obnovení pro zamezení ghostingu
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_BLACK);
  } while (display.nextPage());
  
  // 2. Vyčištění obrazovky na bílou pro "čistý" stav
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());

  // 3. Nastavení stavu a spuštění odpočtu
  g_state = UiState::PreparingForDeepSleep;
  deep_sleep_countdown_start_time = millis();
}

// Nové proměnné pro detekci dlouhého stisku
static unsigned long hold_c_start_time = 0;
static bool is_c_held = false;
static const unsigned long LONG_PRESS_C_DURATION = 2000; // 2 sekundy

void loop() {
    // Pokud probíhá odpočet pro deep sleep, ignorujeme vše ostatní
    if (g_state == UiState::PreparingForDeepSleep) {
      if (millis() - deep_sleep_countdown_start_time > DEEP_SLEEP_CLEAN_DURATION) {
        Serial.println("Countdown finished. Going to deep sleep now.");
        display.powerOff();
        esp_deep_sleep_start();
      }
      delay(100); // Během čekání není potřeba smyčku točit tak rychle
      return;
    }

    // Nová, event-driven logika klávesnice pro detekci dlouhého stisku
    if (keypad.getKeys()) {
        last_key_press_time = millis(); // Resetujeme časovač nečinnosti při jakékoliv aktivitě klávesnice

        for (int i = 0; i < LIST_MAX; i++) {
            if (keypad.key[i].stateChanged) {
                char k = keypad.key[i].kchar;
                Serial.print("Key: ");
                Serial.print(k);
                Serial.print(" state: ");
                Serial.println(keypad.key[i].kstate);

                switch (keypad.key[i].kstate) {
                    case PRESSED:
                        if (k == 'C') {
                            hold_c_start_time = millis();
                            is_c_held = true;
                        } else {
                            // Ostatní klávesy reagují ihned
                            switch (g_state) {
                                case UiState::EnterAmount: onKeyEnter(k); break;
                                case UiState::ShowQr: onKeyQr(k); break;
                            }
                        }
                        break;
                    
                    case RELEASED:
                        if (k == 'C' && is_c_held) {
                            unsigned long press_duration = millis() - hold_c_start_time;
                            if (press_duration > LONG_PRESS_C_DURATION) {
                                // Dlouhý stisk
                                startDeepSleepCountdown();
                            } else {
                                // Krátký stisk
                                if (g_state == UiState::EnterAmount) {
                                    onKeyEnter('C');
                                }
                            }
                            is_c_held = false;
                        }
                        break;
                }
            }
        }
    }

    // Kontrola nečinnosti (pokud nejsme v procesu vypínání)
    if (g_cfg.ui.sleep_timeout_s > 0 && millis() - last_key_press_time > (unsigned long)g_cfg.ui.sleep_timeout_s * 1000) {
        startDeepSleepCountdown();
    }
  
    delay(10); // Lehce zvýšíme delay pro stabilitu event-driven smyčky
}

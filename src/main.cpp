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
  #include <GxEPD2_154_GDEY0154D67.h>
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
  // vždy haléře
  return v;
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

    // Zobrazíme mezisoučet nahoře, pokud nějaký je
    if (running_total_cents > 0) {
      display.setTextSize(2);
      display.setCursor(0, 22);
      String totalStr = formatAmountCz(running_total_cents);
      display.print("Mezisoucet: " + totalStr);
    } else {
      display.setTextSize(2);
      display.setCursor(0, 22);
      display.print(g_cfg.ui.title);
    }

    // Zobrazení stavu baterie v pravém horním rohu
    display.setTextSize(1);
    display.setCursor(display.width() - 40, 5); // Pozice vpravo nahoře
    display.print(String(getBatteryPercentage()) + "%");

    display.setTextSize(3);
    display.setCursor(0, 80);
    display.print(formatAmountForUi());
    display.print(" ");
    display.print(g_cfg.pay.cc.length() ? g_cfg.pay.cc : "CZK");

    display.setTextSize(1);
    display.setCursor(0, 120);
    display.print("A=+ B== C=RST *=DEL #=QR");

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

    display.setTextSize(2);
    display.setCursor(0, 22);
    display.print(amountUi);
    display.print(" ");
    display.print(g_cfg.pay.cc.length() ? g_cfg.pay.cc : "CZK");

    // Zobrazení stavu baterie v pravém horním rohu (přepočítáno na otočený displej)
    // Pokud je displej otočen o 180 (rotation 2), pak (display.width() - X, Y) se stane (X, display.height() - Y)
    display.setTextSize(1);
    display.setCursor(display.width() - 40, 5); // Pozice pro zobrazení na displeji před rotací
    display.print(String(getBatteryPercentage()) + "%");

    display.setTextSize(1);
    display.setCursor(0, display.height() - 10);
    display.print("D=ZPET");

  } while (display.nextPage());
}

enum class UiState {
  EnterAmount,
  ShowQr,
};

static UiState g_state = UiState::EnterAmount;
static String g_lastSpayd;

// ---------- Deep Sleep ----------
static unsigned long last_key_press_time = 0;

static void goEnter() {
  display.setRotation(0);
  g_state = UiState::EnterAmount;
  amountDigits = "";
  running_total_cents = 0;
  is_showing_result = false;
  renderEnterScreen();
}

static void goQr() {
  display.setRotation(2); // otočíme o 180°
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
    // Převedeme celkový výsledek na string, aby se zobrazil jako hlavní částka
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)running_total_cents);
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

void loop() {
  char k = keypad.getKey();
  if (k) {
    last_key_press_time = millis(); // Reset timer on key press
    Serial.print("Key: ");
    Serial.println(k);

    switch (g_state) {
      case UiState::EnterAmount:
        onKeyEnter(k);
        break;
      case UiState::ShowQr:
        onKeyQr(k);
        break;
    }
  } else {
    // Zkontrolujeme nečinnost a případně uspíme
    if (g_cfg.ui.sleep_timeout_s > 0 && millis() - last_key_press_time > (unsigned long)g_cfg.ui.sleep_timeout_s * 1000) {
      Serial.println("Performing full refresh and going to deep sleep...");
      
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

      // 3. Vypnutí displeje a uspání
      display.powerOff();
      esp_deep_sleep_start();
    }
  }
  delay(5); // Krátká pauza pro zamezení vytížení procesoru
}

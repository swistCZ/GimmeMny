#include "app_config.h"

#include <LittleFS.h>

static String trimCopy(String s) {
  s.trim();
  return s;
}

static void applyKeyValue(AppConfig& cfg, const String& section, const String& keyRaw, const String& valueRaw) {
  String key = keyRaw;
  String value = valueRaw;
  key.trim();
  value.trim();

  String sec = section;
  sec.toLowerCase();

  String k = key;
  k.toLowerCase();

  if (sec == "payment") {
    if (k == "acc_iban" || k == "acc") cfg.pay.acc_iban = value;
    else if (k == "cc") cfg.pay.cc = value;
    else if (k == "bic") cfg.pay.bic = value;
    else if (k == "msg") cfg.pay.msg = value;
    else if (k == "rn") cfg.pay.rn = value;
    else if (k == "x-vs" || k == "x_vs") cfg.pay.x_vs = value;
    else if (k == "x-ss" || k == "x_ss") cfg.pay.x_ss = value;
    else if (k == "x-ks" || k == "x_ks") cfg.pay.x_ks = value;
  } else if (sec == "ui") {
    if (k == "title") cfg.ui.title = value;
    else if (k == "translit") cfg.ui.translit = (value == "1" || value == "true" || value == "yes");
    else if (k == "sleep_timeout_s") cfg.ui.sleep_timeout_s = value.toInt();
  }
}

AppConfig defaultConfig() {
  AppConfig cfg;
  cfg.pay.acc_iban = "CZ5855000000001265098001";
  cfg.pay.cc = "CZK";
  cfg.pay.bic = "";
  cfg.pay.msg = "";
  cfg.pay.rn = "";
  cfg.pay.x_vs = "";
  cfg.pay.x_ss = "";
  cfg.pay.x_ks = "";

  cfg.ui.title = "Kalkuvacka";
  cfg.ui.translit = true;
  cfg.ui.sleep_timeout_s = 60;

  return cfg;
}

bool loadConfigFromFs(AppConfig& cfg) {
  File f = LittleFS.open("/config.ini", "r");
  if (!f) return false;

  String section;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;
    if (line.startsWith("#") || line.startsWith(";")) continue;

    // inline komentáře (jen jednoduché)
    int hashPos = line.indexOf('#');
    if (hashPos >= 0) {
      line = trimCopy(line.substring(0, hashPos));
      if (line.length() == 0) continue;
    }

    if (line.startsWith("[") && line.endsWith("]")) {
      section = line.substring(1, line.length() - 1);
      section.trim();
      continue;
    }

    int eq = line.indexOf('=');
    if (eq < 0) continue;

    String key = line.substring(0, eq);
    String value = line.substring(eq + 1);
    applyKeyValue(cfg, section, key, value);
  }

  f.close();
  return true;
}

static void writeKV(File& f, const char* key, const String& value) {
  f.print(key);
  f.print('=');
  f.println(value);
}

bool saveConfigToFs(const AppConfig& cfg) {
  File f = LittleFS.open("/config.ini", "w");
  if (!f) return false;

  f.println("[payment]");
  writeKV(f, "acc_iban", cfg.pay.acc_iban);
  writeKV(f, "cc", cfg.pay.cc);
  writeKV(f, "bic", cfg.pay.bic);
  writeKV(f, "msg", cfg.pay.msg);
  writeKV(f, "rn", cfg.pay.rn);
  writeKV(f, "x_vs", cfg.pay.x_vs);
  writeKV(f, "x_ss", cfg.pay.x_ss);
  writeKV(f, "x_ks", cfg.pay.x_ks);
  f.println();

  f.println("[ui]");
  writeKV(f, "title", cfg.ui.title);
  writeKV(f, "translit", cfg.ui.translit ? "true" : "false");
  writeKV(f, "sleep_timeout_s", String(cfg.ui.sleep_timeout_s));

  f.close();
  return true;
}

void printConfig(const AppConfig& cfg, Stream& out) {
  out.println("--- config ---");
  out.print("acc_iban="); out.println(cfg.pay.acc_iban);
  out.print("cc="); out.println(cfg.pay.cc);
  out.print("bic="); out.println(cfg.pay.bic);
  out.print("msg="); out.println(cfg.pay.msg);
  out.print("rn="); out.println(cfg.pay.rn);
  out.print("x_vs="); out.println(cfg.pay.x_vs);
  out.print("x_ss="); out.println(cfg.pay.x_ss);
  out.print("x_ks="); out.println(cfg.pay.x_ks);
  out.print("title="); out.println(cfg.ui.title);
  out.print("translit="); out.println(cfg.ui.translit ? "true" : "false");
  out.print("sleep_timeout_s="); out.println(cfg.ui.sleep_timeout_s);
  out.println("--------------");
}

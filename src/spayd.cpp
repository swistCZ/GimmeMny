#include "spayd.h"

static String toUpperAscii(String s) {
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c >= 'a' && c <= 'z') s.setCharAt(i, (char)(c - 32));
  }
  return s;
}

// Hrubá transliterace CZ diakritiky pro MSG/RN (UTF-8).
// Pozn.: Dělá jen nejběžnější znaky.
static String translitCz(String s) {
  s.replace("á", "a"); s.replace("č", "c"); s.replace("ď", "d"); s.replace("é", "e"); s.replace("ě", "e");
  s.replace("í", "i"); s.replace("ň", "n"); s.replace("ó", "o"); s.replace("ř", "r"); s.replace("š", "s");
  s.replace("ť", "t"); s.replace("ú", "u"); s.replace("ů", "u"); s.replace("ý", "y"); s.replace("ž", "z");
  s.replace("Á", "A"); s.replace("Č", "C"); s.replace("Ď", "D"); s.replace("É", "E"); s.replace("Ě", "E");
  s.replace("Í", "I"); s.replace("Ň", "N"); s.replace("Ó", "O"); s.replace("Ř", "R"); s.replace("Š", "S");
  s.replace("Ť", "T"); s.replace("Ú", "U"); s.replace("Ů", "U"); s.replace("Ý", "Y"); s.replace("Ž", "Z");
  return s;
}

static bool isUnreserved(uint8_t c) {
  // pro jednoduchost: necháme A-Z a-z 0-9 a pár safe znaků
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) return true;
  switch (c) {
    case '-': case '.': case '_': case '~':
    case ' ': case ',': case '/': case ':': case ';': case '+':
      return true;
    default:
      return false;
  }
}

static String percentEncodeUtf8(const String& s) {
  String out;
  out.reserve(s.length() * 3);

  const uint8_t* bytes = (const uint8_t*)s.c_str();
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = bytes[i];

    // '*' je separator v SPD, ten musíme vždy enkódovat
    if (c == '*') {
      out += "%2A";
      continue;
    }

    if (isUnreserved(c)) {
      out += (char)c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    }
  }
  return out;
}

String formatAmountCz(int64_t amountCents) {
  if (amountCents < 0) amountCents = 0;

  int64_t crowns = amountCents / 100;
  int64_t cents = amountCents % 100;

  char buf[32];
  snprintf(buf, sizeof(buf), "%lld.%02lld", (long long)crowns, (long long)cents);
  return String(buf);
}

static void appendField(String& s, const char* key, const String& value) {
  if (value.length() == 0) return;
  s += '*';
  s += key;
  s += ':';
  s += value;
}

String buildSpayd(const AppConfig& cfg, int64_t amountCents) {
  String s = "SPD*1.0";

  // ACC je povinné
  String acc = cfg.pay.acc_iban;
  acc.trim();
  appendField(s, "ACC", acc);

  if (cfg.pay.bic.length()) {
    // SPAYD v praxi používá BIC jako vlastní field
    appendField(s, "BIC", toUpperAscii(cfg.pay.bic));
  }

  // částka
  if (amountCents > 0) {
    appendField(s, "AM", formatAmountCz(amountCents));
  }

  String cc = cfg.pay.cc.length() ? cfg.pay.cc : "CZK";
  appendField(s, "CC", toUpperAscii(cc));

  // volitelné identifikátory
  appendField(s, "X-VS", cfg.pay.x_vs);
  appendField(s, "X-SS", cfg.pay.x_ss);
  appendField(s, "X-KS", cfg.pay.x_ks);

  // texty – doporučení: bez diakritiky + uppercase, plus percent-encoding
  if (cfg.pay.rn.length()) {
    String rn = cfg.ui.translit ? translitCz(cfg.pay.rn) : cfg.pay.rn;
    rn = toUpperAscii(rn);
    appendField(s, "RN", percentEncodeUtf8(rn));
  }
  if (cfg.pay.msg.length()) {
    String msg = cfg.ui.translit ? translitCz(cfg.pay.msg) : cfg.pay.msg;
    msg = toUpperAscii(msg);
    appendField(s, "MSG", percentEncodeUtf8(msg));
  }

  return s;
}

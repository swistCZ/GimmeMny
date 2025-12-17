#pragma once

#include <Arduino.h>
#include "app_config.h"

// amountCents = částka v haléřích (např. 12345 => 123.45)
String formatAmountCz(int64_t amountCents);

// Sestaví SPD řetězec pro českou QR platbu.
String buildSpayd(const AppConfig& cfg, int64_t amountCents);

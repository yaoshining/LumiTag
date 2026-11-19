#pragma once
#include <Arduino.h>
#include "WifiLocationFix.h"

// One-shot diagnostic only; never publishes a location.
void probeAliyunWifiLocation(const String& product, const String& device, const String& secret, bool privateDisplay, WifiLocationFix* output = nullptr);

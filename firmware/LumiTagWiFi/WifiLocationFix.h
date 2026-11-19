#pragma once
#include <cmath>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

struct WifiLocationFix {
  double latitude = 0, longitude = 0;
  int coordinateSystem = 0;
  uint32_t scanStartedMs = 0;
  char capturedAtUtc[21] = {};
  bool valid = false;
};

inline bool wifiLocationUsable(const WifiLocationFix& fix, uint32_t nowMs) {
  return fix.valid && std::isfinite(fix.latitude) && std::isfinite(fix.longitude) &&
      fix.latitude >= -90 && fix.latitude <= 90 && fix.longitude >= -180 && fix.longitude <= 180 &&
      (fix.latitude != 0 || fix.longitude != 0) &&
      (fix.coordinateSystem == 1 || fix.coordinateSystem == 2) &&
      strlen(fix.capturedAtUtc) == 20 && static_cast<uint32_t>(nowMs - fix.scanStartedMs) <= 60000;
}

inline bool formatWifiLocation(const WifiLocationFix& fix, uint32_t nowMs, char* output, size_t capacity) {
  if (!output || !capacity || !wifiLocationUsable(fix, nowMs)) return false;
  const char* system = fix.coordinateSystem == 2 ? "GCJ-02" : "WGS-84";
  int size = snprintf(output, capacity,
      "{\"latitude\":%.6f,\"longitude\":%.6f,\"capturedAtUtc\":\"%s\",\"source\":\"wifi\",\"coordinateSystem\":\"%s\",\"accuracyM\":null,\"accuracyStatus\":\"unknown\",\"estimate\":true}",
      fix.latitude, fix.longitude, fix.capturedAtUtc, system);
  return size >= 0 && static_cast<size_t>(size) < capacity;
}

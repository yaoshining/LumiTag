#include "WifiFingerprint.h"
#include <WiFi.h>
#include <string.h>

void scanWifiFingerprint() {
  const unsigned kMaxPoints = 30;
  uint8_t ids[kMaxPoints][6] = {};
  unsigned unique = 0;
  Serial.println("Wi-Fi location scan starting; no data will be uploaded.");
  const int found = WiFi.scanNetworks(false, false, false, 200);
  if (found < 0) {
    WiFi.scanDelete();
    Serial.println("Wi-Fi location scan failed; no position available.");
    return;
  }
  for (int i = 0; i < found && i < 256 && unique < kMaxPoints; ++i) {
    const uint8_t *id = WiFi.BSSID(static_cast<uint8_t>(i));
    if (!id || (id[0] & 1)) continue;
    bool empty = true, duplicate = false;
    for (unsigned b = 0; b < 6; ++b) if (id[b]) empty = false;
    for (unsigned j = 0; j < unique; ++j) if (memcmp(ids[j], id, 6) == 0) duplicate = true;
    if (empty || duplicate) continue;
    memcpy(ids[unique++], id, 6);
  }
  WiFi.scanDelete();
  volatile uint8_t *wipe = &ids[0][0];
  for (unsigned i = 0; i < sizeof(ids); ++i) wipe[i] = 0;
  Serial.printf("Wi-Fi location scan: %d detected, %u distinct unicast observations (limit %u).\n", found, unique, kMaxPoints);
  Serial.println("No location: geolocation service is not configured. Observations discarded.");
}

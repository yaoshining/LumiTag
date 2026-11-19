#include "../LumiTagWiFi/WifiLocationFix.h"
#include <assert.h>
#include <limits>

int main() {
  WifiLocationFix fix;
  assert(!wifiLocationUsable(fix, 10));
  fix.valid = true; fix.latitude = 30.0; fix.longitude = 120.0;
  fix.coordinateSystem = 2; fix.scanStartedMs = 100;
  strcpy(fix.capturedAtUtc, "2026-11-19T00:00:00Z");
  assert(wifiLocationUsable(fix, 60100));
  char payload[320];
  assert(formatWifiLocation(fix, 101, payload, sizeof(payload)));
  assert(strstr(payload, "\"source\":\"wifi\"") && strstr(payload, "\"coordinateSystem\":\"GCJ-02\""));
  assert(strstr(payload, "\"accuracyM\":null") && strstr(payload, "\"accuracyStatus\":\"unknown\""));
  assert(!formatWifiLocation(fix, 60101, payload, sizeof(payload)));
  assert(!formatWifiLocation(fix, 101, payload, 5));
  assert(!wifiLocationUsable(fix, 60101));
  fix.coordinateSystem = 0; assert(!wifiLocationUsable(fix, 101));
  fix.coordinateSystem = 1; assert(wifiLocationUsable(fix, 101));
  fix.latitude = std::numeric_limits<double>::quiet_NaN(); assert(!wifiLocationUsable(fix, 101));
  fix.latitude = 91; assert(!wifiLocationUsable(fix, 101));
  fix.latitude = 0; fix.longitude = 0; assert(!wifiLocationUsable(fix, 101));
  fix.latitude = 30; fix.longitude = 120; fix.capturedAtUtc[0] = '\0'; assert(!wifiLocationUsable(fix, 101));
  strcpy(fix.capturedAtUtc, "2026-11-19T00:00:00Z");
  fix.scanStartedMs = 0xfffffff0u; assert(wifiLocationUsable(fix, 10));
}

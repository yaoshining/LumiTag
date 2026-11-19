#include "AliyunWifiProbe.h"
#include "AliyunHttpsRoot.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <mbedtls/md.h>
#include <time.h>

namespace {
const char* const kHost = "https://iot-as-http.cn-shanghai.aliyuncs.com";

bool request(const String& path, const String& body, const char* type,
             const String& token, String& response, int& status) {
  WiFiClientSecure transport;
  transport.setCACert(kAliyunHttpsRoot);
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(transport, String(kHost) + path)) return false;
  http.addHeader("Content-Type", type);
  if (token.length()) http.addHeader("password", token);
  status = http.POST(body);
  if (status > 0 && http.getSize() <= 4096) response = http.getString();
  http.end();
  return status > 0;
}

bool validName(const String& value) {
  if (!value.length() || value.length() > 64) return false;
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];
    if (!isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != '.' && c != ':') return false;
  }
  return true;
}
}

void probeAliyunWifiLocation(const String& product, const String& device, const String& secret, bool privateDisplay, WifiLocationFix* output) {
  if (output) *output = WifiLocationFix{};
  if (WiFi.status() != WL_CONNECTED) { Serial.println("Wi-Fi location probe failed: Wi-Fi disconnected."); return; }
  if (!validName(product) || !validName(device) || !secret.length()) { Serial.println("Wi-Fi location probe failed: invalid cloud credentials."); return; }
  if (time(nullptr) <= 1700000000) { Serial.println("Wi-Fi location probe failed: clock not synchronized; run cloud probe after connecting Wi-Fi."); return; }

  String observations;
  uint8_t seen[30][6] = {};
  unsigned count = 0;
  const uint32_t scanStartedMs = millis();
  const time_t scannedAt = time(nullptr);
  Serial.println("Wi-Fi location probe: scanning; only status will be printed.");
  int found = WiFi.scanNetworks(false, false, false, 200);
  if (found < 0) { WiFi.scanDelete(); Serial.println("Wi-Fi location probe failed: scan error."); return; }
  for (int i = 0; i < found && i < 256 && count < 30; ++i) {
    const uint8_t* mac = WiFi.BSSID(static_cast<uint8_t>(i));
    String ssid = WiFi.SSID(i);
    if (!mac || (mac[0] & 1) || !ssid.length() || ssid.indexOf(',') >= 0 || ssid.indexOf('|') >= 0) continue;
    bool duplicate = false, empty = true;
    for (unsigned b = 0; b < 6; ++b) if (mac[b]) empty = false;
    for (unsigned j = 0; j < count; ++j) if (!memcmp(seen[j], mac, 6)) duplicate = true;
    if (duplicate || empty) continue;
    char id[18];
    snprintf(id, sizeof(id), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (count) observations += '|';
    observations += String(id) + ',' + String(WiFi.RSSI(i)) + ',' + ssid;
    memcpy(seen[count++], mac, 6);
  }
  WiFi.scanDelete();
  memset(seen, 0, sizeof(seen));
  if (count < 2) { Serial.println("Wi-Fi location probe failed: fewer than two distinct access points."); return; }
  Serial.printf("Wi-Fi location probe: %u observations; mobile hotspots cannot be identified automatically.\n", count);

  String stamp = String(static_cast<unsigned long>(time(nullptr))) + "000";
  String signing = "clientIdlumitag-location-probedeviceName" + device + "productKey" + product + "timestamp" + stamp;
  unsigned char digest[20];
  const mbedtls_md_info_t* algorithm = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  if (!algorithm || mbedtls_md_hmac(algorithm, reinterpret_cast<const unsigned char*>(secret.c_str()), secret.length(),
      reinterpret_cast<const unsigned char*>(signing.c_str()), signing.length(), digest) != 0) {
    Serial.println("Wi-Fi location probe failed: signature error."); return;
  }
  char signature[41];
  for (unsigned i = 0; i < 20; ++i) snprintf(signature + i * 2, 3, "%02x", digest[i]);
  memset(digest, 0, sizeof(digest));
  StaticJsonDocument<1024> auth;
  auth["clientId"] = "lumitag-location-probe"; auth["deviceName"] = device; auth["productKey"] = product;
  auth["timestamp"] = stamp; auth["signmethod"] = "hmacsha1"; auth["sign"] = signature; auth["version"] = "default";
  String body, response;
  serializeJson(auth, body);
  memset(signature, 0, sizeof(signature));
  int status = 0;
  if (!request("/auth", body, "application/json", "", response, status)) { Serial.println("Wi-Fi location probe failed: HTTPS authentication transport or certificate error."); return; }
  body = "";
  StaticJsonDocument<2048> result;
  if (deserializeJson(result, response) || status != 200 || result["code"].as<int>() != 0 || !result["info"]["token"].is<const char*>()) {
    Serial.printf("Wi-Fi location probe: authentication failed, HTTP %d.\n", status); return;
  }
  String token = result["info"]["token"].as<String>();
  result.clear(); response = "";
  DynamicJsonDocument location(8192);
  location["id"] = "1"; location["version"] = "1.0"; location["method"] = "_thing.service.post";
  location["params"]["identifier"] = "Location.Position";
  location["params"]["serviceParams"]["type"] = "wifi";
  location["params"]["serviceParams"]["macs"] = observations;
  if (location.overflowed()) { Serial.println("Wi-Fi location probe failed: observation buffer exhausted."); return; }
  serializeJson(location, body);
  observations = "";
  String path = "/topic/sys/" + product + "/" + device + "/_thing/service/post";
  if (!request(path, body, "application/octet-stream", token, response, status)) { Serial.println("Wi-Fi location probe failed: HTTPS location transport or certificate error."); return; }
  body = ""; token = "";
  if (deserializeJson(result, response)) { Serial.printf("Wi-Fi location probe: invalid response, HTTP %d.\n", status); return; }
  int code = result["code"] | -1;
  if (status == 200 && code == 0 && result["info"]["latitude"].is<double>() && result["info"]["longitude"].is<double>()) {
    double lat = result["info"]["latitude"], lon = result["info"]["longitude"];
    int system = result["info"]["CoordinateSystem"] | 0;
    if (lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180 && (lat != 0 || lon != 0) && (system == 1 || system == 2)) {
      if (output) {
        struct tm utc;
        if (!gmtime_r(&scannedAt, &utc) || !strftime(output->capturedAtUtc, sizeof(output->capturedAtUtc), "%Y-%m-%dT%H:%M:%SZ", &utc)) {
          Serial.println("Wi-Fi location probe failed: invalid capture time."); return;
        }
        output->latitude = lat; output->longitude = lon; output->coordinateSystem = system;
        output->scanStartedMs = scanStartedMs; output->valid = true;
      }
      Serial.printf("Wi-Fi location probe: valid coordinates returned, coordinate system %d; accuracy unavailable.\n", system);
      if (privateDisplay) {
        Serial.printf("Private local result: latitude %.7f, longitude %.7f\n", lat, lon);
        Serial.println("Do not save or share this result. Accuracy radius unavailable.");
      } else {
        Serial.println("Diagnostic: coordinates are not printed.");
      }
      return;
    }
  }
  Serial.printf("Wi-Fi location probe failed: HTTP %d, code %d.\n", status, code);
  Serial.println("Check service entitlement, GeoLocation model and observations; failure is not proof of missing entitlement.");
}

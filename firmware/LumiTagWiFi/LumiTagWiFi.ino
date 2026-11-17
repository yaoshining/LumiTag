#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "mbedtls/md.h"
#include <time.h>
#include <sys/time.h>

namespace {
const unsigned long kConnectTimeoutMs = 20000;
const char* kMqttHost = "iot-06z00bdz2tpu3so.mqtt.iothub.aliyuncs.com";
const uint16_t kMqttPort = 8883;

// Alibaba Cloud IoT PCA root for MQTT over port 8883.
const char kAlibabaIotRootCa[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIID3zCCAsegAwIBAgISfiX6mTa5RMUTGSC3rQhnestIMA0GCSqGSIb3DQEBCwUA
MHcxCzAJBgNVBAYTAkNOMREwDwYDVQQIDAhaaGVqaWFuZzERMA8GA1UEBwwISGFu
Z3pob3UxEzARBgNVBAoMCkFsaXl1biBJb1QxEDAOBgNVBAsMB1Jvb3QgQ0ExGzAZ
BgNVBAMMEkFsaXl1biBJb1QgUm9vdCBDQTAgFw0yMzA3MDQwNjM2NThaGA8yMDUz
MDcwNDA2MzY1OFowdzELMAkGA1UEBhMCQ04xETAPBgNVBAgMCFpoZWppYW5nMREw
DwYDVQQHDAhIYW5nemhvdTETMBEGA1UECgwKQWxpeXVuIElvVDEQMA4GA1UECwwH
Um9vdCBDQTEbMBkGA1UEAwwSQWxpeXVuIElvVCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoK//6vc2oXhnvJD7BVhj6grj7PMlN2N4iNH4
GBmLmMdkF1z9eQLjksYc4Zid/FX67ypWFtdycOei5ec0X00m53Gvy4zLGBo2uKgi
T9IxMudmt95bORZbaph4VK82gPNU4ewbiI1q2loRZEHRdyPORTPpvNLHu8DrYBnY
Vg5feEYLLyhxg5M1UTrT/30RggHpaa0BYIPxwsKyylQ1OskOsyZQeOyPe8t8r2D4
RBpUGc5ix4j537HYTKSyK3Hv57R7w1NzKtXoOioDOm+YySsz9sTLFajZkUcQci4X
aedyEeguDLAIUKiYicJhRCZWljVlZActorTgjCY4zRajodThrQIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUkWHoKi2h
DlS1/rYpcT/Ue+aKhP8wHwYDVR0jBBgwFoAUkWHoKi2hDlS1/rYpcT/Ue+aKhP8w
DQYJKoZIhvcNAQELBQADggEBADrrLcBY7gDXN8/0KHvPbGwMrEAJcnF9z4MBxRvt
rEoRxhlvRZzPi7w/868xbipwwnksZsn0QNIiAZ6XzbwvIFG01ONJET+OzDy6ZqUb
YmJI09EOe9/Hst8Fac2D14Oyw0+6KTqZW7WWrP2TAgv8/Uox2S05pCWNfJpRZxOv
Lr4DZmnXBJCMNMY/X7xpcjylq+uCj118PBobfH9Oo+iAJ4YyjOLmX3bflKIn1Oat
vdJBtXCj3phpfuf56VwKxoxEVR818GqPAHnz9oVvye4sQqBp/2ynrKFxZKUaJtk0
7UeVbtecwnQTrlcpWM7ACQC0OO0M9+uNjpKIbksv1s11xu0=
-----END CERTIFICATE-----
)EOF";

String networkSsid;
String networkPassword;
String productKey;
String deviceName;
String deviceSecret;

WiFiClientSecure mqttTransport;
PubSubClient mqttClient(mqttTransport);

const char* encryptionName(wifi_auth_mode_t encryption) {
  switch (encryption) {
    case WIFI_AUTH_OPEN: return "open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    default: return "unknown";
  }
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help                         Show this message");
  Serial.println("  scan                         Scan nearby Wi-Fi networks");
  Serial.println("  ssid <name>                  Set the network name in RAM");
  Serial.println("  password <value>             Set the Wi-Fi password in RAM");
  Serial.println("  connect                      Connect Wi-Fi with current settings");
  Serial.println("  status                       Show Wi-Fi status");
  Serial.println("  disconnect                   Disconnect Wi-Fi and MQTT");
  Serial.println("  cloud help                   Show cloud commands");
  Serial.println("  cloud product <ProductKey>   Set the Alibaba Cloud ProductKey");
  Serial.println("  cloud device <DeviceName>    Set the Alibaba Cloud DeviceName");
  Serial.println("  cloud secret <DeviceSecret>  Set the device secret in RAM only");
  Serial.println("  cloud probe                  Diagnose DNS, TCP, and TLS connectivity");
  Serial.println("  cloud connect                Connect to Alibaba Cloud MQTT");
  Serial.println("  cloud status                 Show MQTT status");
  Serial.println("  cloud location <lat> <lon>   Publish a telemetry JSON message");
  Serial.println("  cloud disconnect             Disconnect MQTT");
}

void printCloudHelp() {
  Serial.println("Alibaba Cloud MQTT uses TLS on port 8883 with server certificate validation.");
  Serial.println("The firmware publishes to /<ProductKey>/<DeviceName>/user/update.");
  Serial.println("The product's built-in custom update topic has publish permission.");
  Serial.println("The device secret is never printed or stored in flash.");
}

void scanNetworks() {
  Serial.println("Scanning Wi-Fi networks...");
  const int count = WiFi.scanNetworks();
  if (count <= 0) {
    Serial.println("No networks found.");
    return;
  }
  Serial.printf("Found %d network(s):\n", count);
  for (int index = 0; index < count; ++index) {
    Serial.printf("  %d. %s (%d dBm, %s)\n", index + 1,
                  WiFi.SSID(index).c_str(), WiFi.RSSI(index),
                  encryptionName(WiFi.encryptionType(index)));
  }
  WiFi.scanDelete();
}

void printStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi is disconnected.");
    return;
  }
  Serial.printf("Connected to: %s\n", WiFi.SSID().c_str());
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
}

void connectNetwork() {
  if (networkSsid.length() == 0) {
    Serial.println("Set an SSID first: ssid <name>");
    return;
  }
  mqttClient.disconnect();
  WiFi.disconnect(true);
  Serial.printf("Connecting to %s", networkSsid.c_str());
  WiFi.begin(networkSsid.c_str(), networkPassword.c_str());
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < kConnectTimeoutMs) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected.");
    printStatus();
  } else {
    Serial.println("Connection timed out. Check SSID, password, and 2.4 GHz coverage.");
  }
}

bool cloudCredentialsReady() {
  if (productKey.length() == 0 || deviceName.length() == 0 || deviceSecret.length() == 0) {
    Serial.println("Set cloud product, device, and secret first.");
    return false;
  }
  return true;
}

bool synchronizeClock() {
  if (time(NULL) > 1700000000) {
    return true;
  }

  Serial.println("Synchronizing time for MQTT authentication...");
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com");
  const unsigned long startedAt = millis();
  while (time(NULL) <= 1700000000 && millis() - startedAt < 10000) {
    delay(250);
  }
  if (time(NULL) > 1700000000) {
    return true;
  }

  // Some Wi-Fi networks block UDP/NTP. The timestamp is only used for MQTT
  // request freshness; the MQTT session itself remains CA-validated TLS.
  Serial.println("NTP unavailable; trying Alibaba HTTP time fallback...");
  WiFiClient timeClient;
  timeClient.setTimeout(5000);
  if (!timeClient.connect("api.m.taobao.com", 80)) {
    Serial.println("HTTP time fallback connection failed.");
    return false;
  }
  timeClient.print("GET /rest/api3.do?api=mtop.common.getTimestamp HTTP/1.1\r\n"
                   "Host: api.m.taobao.com\r\nConnection: close\r\n\r\n");

  String response;
  response.reserve(768);
  const unsigned long responseStartedAt = millis();
  while (millis() - responseStartedAt < 5000) {
    while (timeClient.available() > 0 && response.length() < 2048) {
      response += static_cast<char>(timeClient.read());
    }
    if (!timeClient.connected()) break;
    delay(20);
  }
  timeClient.stop();

  const int timestampStart = response.indexOf("\"t\":\"");
  if (timestampStart < 0) {
    Serial.println("HTTP time fallback returned no timestamp.");
    return false;
  }
  const int valueStart = timestampStart + 5;
  const int valueEnd = response.indexOf('"', valueStart);
  if (valueEnd < 0) {
    Serial.println("HTTP time fallback returned an invalid timestamp.");
    return false;
  }
  const unsigned long long milliseconds = strtoull(
      response.substring(valueStart, valueEnd).c_str(), NULL, 10);
  if (milliseconds < 1700000000000ULL) {
    Serial.println("HTTP time fallback returned an out-of-range timestamp.");
    return false;
  }
  const timeval clockValue = {
      static_cast<time_t>(milliseconds / 1000),
      static_cast<suseconds_t>((milliseconds % 1000) * 1000),
  };
  settimeofday(&clockValue, NULL);
  const bool synchronized = time(NULL) > 1700000000;
  Serial.println(synchronized ? "Clock synchronized via HTTP fallback."
                              : "HTTP time fallback could not set the clock.");
  return synchronized;
}

String mqttTimestamp() {
  char timestamp[16];
  snprintf(timestamp, sizeof(timestamp), "%llu000",
           static_cast<unsigned long long>(time(NULL)));
  return String(timestamp);
}

bool hmacSha256(const String& secret, const String& source, char output[65]) {
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  unsigned char digest[32];
  if (info == NULL || mbedtls_md_hmac(info,
      reinterpret_cast<const unsigned char*>(secret.c_str()), secret.length(),
      reinterpret_cast<const unsigned char*>(source.c_str()), source.length(), digest) != 0) {
    return false;
  }
  for (int index = 0; index < 32; ++index) {
    sprintf(output + index * 2, "%02x", digest[index]);
  }
  output[64] = '\0';
  return true;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Cloud message on %s: ", topic);
  for (unsigned int index = 0; index < length; ++index) {
    Serial.write(payload[index]);
  }
  Serial.println();
}

void printCloudStatus() {
  Serial.printf("MQTT: %s\n", mqttClient.connected() ? "connected" : "disconnected");
  if (!mqttClient.connected()) {
    Serial.printf("Last MQTT state: %d\n", mqttClient.state());
  }
}

void printTlsError() {
  char error[128] = {0};
  const int errorCode = mqttTransport.lastError(error, sizeof(error));
  Serial.printf("TLS error: %d (%s)\n", errorCode, error[0] == '\0' ? "no detail" : error);
}

void probeCloudEndpoint() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connect Wi-Fi first.");
    return;
  }

  IPAddress endpoint;
  if (WiFi.hostByName(kMqttHost, endpoint) != 1) {
    Serial.println("DNS lookup failed for the Alibaba Cloud MQTT endpoint.");
    return;
  }
  Serial.printf("DNS resolved: %s\n", endpoint.toString().c_str());

  WiFiClient tcpProbe;
  tcpProbe.setTimeout(5000);
  if (!tcpProbe.connect(kMqttHost, kMqttPort)) {
    Serial.println("TCP connection to port 8883 failed.");
    return;
  }
  Serial.println("TCP connection to port 8883 succeeded.");
  tcpProbe.stop();

  mqttTransport.stop();
  mqttTransport.setCACert(kAlibabaIotRootCa);
  mqttTransport.setHandshakeTimeout(10);
  if (!mqttTransport.connect(kMqttHost, kMqttPort)) {
    Serial.println("TLS handshake or certificate validation failed.");
    printTlsError();
    return;
  }
  Serial.println("TLS handshake and certificate validation succeeded.");
  mqttTransport.stop();
}

void connectCloud() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connect Wi-Fi first.");
    return;
  }
  if (!cloudCredentialsReady()) return;
  if (!synchronizeClock()) {
    Serial.println("Clock synchronization failed. Check Wi-Fi Internet access.");
    return;
  }

  const String timestamp = mqttTimestamp();
  const String clientIdValue = productKey + "." + deviceName;
  const String signatureSource = "clientId" + clientIdValue + "deviceName" + deviceName +
                                 "productKey" + productKey + "timestamp" + timestamp;
  char password[65];
  if (!hmacSha256(deviceSecret, signatureSource, password)) {
    Serial.println("Unable to calculate the MQTT signature.");
    return;
  }
  const String mqttClientId = clientIdValue + "|securemode=2,signmethod=hmacsha256,timestamp=" + timestamp + "|";
  const String username = deviceName + "&" + productKey;
  mqttTransport.setCACert(kAlibabaIotRootCa);
  mqttTransport.setHandshakeTimeout(10);
  mqttClient.setServer(kMqttHost, kMqttPort);
  mqttClient.setCallback(mqttCallback);
  Serial.println("Connecting to Alibaba Cloud MQTT over TLS with server certificate validation...");
  if (mqttClient.connect(mqttClientId.c_str(), username.c_str(), password)) {
    Serial.println("Alibaba Cloud MQTT connected.");
  } else {
    Serial.printf("MQTT connection failed, state: %d\n", mqttClient.state());
    printTlsError();
  }
}

void publishLocation(const String& arguments) {
  if (!mqttClient.connected()) {
    Serial.println("Connect Alibaba Cloud MQTT first.");
    return;
  }
  const int separator = arguments.indexOf(' ');
  if (separator < 0) {
    Serial.println("Usage: cloud location <latitude> <longitude>");
    return;
  }
  const String latitude = arguments.substring(0, separator);
  String longitude = arguments.substring(separator + 1);
  longitude.trim();
  if (latitude.length() == 0 || longitude.length() == 0) {
    Serial.println("Usage: cloud location <latitude> <longitude>");
    return;
  }
  const String topic = "/" + productKey + "/" + deviceName + "/user/update";
  const String payload = "{\"latitude\":" + latitude + ",\"longitude\":" + longitude +
                         ",\"uptimeMs\":" + String(millis()) + "}";
  if (mqttClient.publish(topic.c_str(), payload.c_str())) {
    Serial.println("Telemetry published.");
  } else {
    Serial.println("Telemetry publish failed.");
  }
}

void handleCloudCommand(const String& command) {
  String normalizedCommand = command;
  normalizedCommand.toLowerCase();
  if (normalizedCommand == "cloud help") printCloudHelp();
  else if (normalizedCommand == "cloud probe") probeCloudEndpoint();
  else if (normalizedCommand == "cloud connect") connectCloud();
  else if (normalizedCommand == "cloud status") printCloudStatus();
  else if (normalizedCommand == "cloud disconnect") {
    mqttClient.disconnect();
    Serial.println("Alibaba Cloud MQTT disconnected.");
  } else if (normalizedCommand.startsWith("cloud product ")) {
    productKey = command.substring(14);
    productKey.trim();
    Serial.println("ProductKey received.");
  } else if (normalizedCommand.startsWith("cloud device ")) {
    deviceName = command.substring(13);
    deviceName.trim();
    Serial.println("DeviceName received.");
  } else if (normalizedCommand.startsWith("cloud secret ")) {
    deviceSecret = command.substring(13);
    deviceSecret.trim();
    Serial.println("Device secret received and kept in RAM only.");
  } else if (normalizedCommand.startsWith("cloud location ")) {
    publishLocation(command.substring(15));
  } else Serial.println("Unknown cloud command. Type cloud help.");
}

void handleCommand(String command) {
  command.trim();
  if (command.length() == 0) return;
  String normalizedCommand = command;
  normalizedCommand.toLowerCase();
  if (normalizedCommand.startsWith("cloud ")) handleCloudCommand(command);
  else if (normalizedCommand == "help") printHelp();
  else if (normalizedCommand == "scan") scanNetworks();
  else if (normalizedCommand == "connect") connectNetwork();
  else if (normalizedCommand == "status") printStatus();
  else if (normalizedCommand == "disconnect") {
    mqttClient.disconnect();
    WiFi.disconnect(true);
    Serial.println("Wi-Fi and MQTT disconnected.");
  } else if (normalizedCommand.startsWith("ssid ")) {
    networkSsid = command.substring(5);
    networkSsid.trim();
    Serial.printf("SSID set to: %s\n", networkSsid.c_str());
  } else if (normalizedCommand.startsWith("password ")) {
    networkPassword = command.substring(9);
    Serial.println("Wi-Fi password received and kept in RAM only.");
  } else Serial.println("Unknown command. Type help.");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(300);
  Serial.println();
  Serial.println("LumiTag Wi-Fi and Alibaba Cloud connectivity test");
  Serial.println("Credentials are never stored in flash.");
  printHelp();
  scanNetworks();
}

void loop() {
  mqttClient.loop();
  if (Serial.available() > 0) handleCommand(Serial.readStringUntil('\n'));
}

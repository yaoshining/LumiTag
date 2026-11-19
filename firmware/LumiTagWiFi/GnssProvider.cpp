#include "GnssProvider.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

namespace {
const unsigned long kTimeout = 180000, kLifetime = 120000;
bool number(const String &s, double &v) {
  if (!s.length()) return false;
  char *end = nullptr;
  v = strtod(s.c_str(), &end);
  return end != s.c_str() && *end == '\0' && std::isfinite(v);
}
bool coordinate(const String &s, const String &dir, bool lat, double &out) {
  double raw;
  if (!number(s, raw) || raw < 0) return false;
  int deg = static_cast<int>(raw / 100);
  double min = raw - deg * 100;
  if (min >= 60 || deg > (lat ? 90 : 180) ||
      (deg == (lat ? 90 : 180) && min != 0)) return false;
  if (dir != (lat ? "N" : "E") && dir != (lat ? "S" : "W")) return false;
  out = (deg + min / 60) * ((dir == "S" || dir == "W") ? -1 : 1);
  return true;
}
bool utc(const String &date, const String &clock, char *out, int &secondOfDay) {
  if (date.length() != 6 || clock.length() < 6) return false;
  for (int i = 0; i < 6; ++i) if (!isDigit(date[i]) || !isDigit(clock[i])) return false;
  if (clock.length() > 6 && clock[6] != '.') return false;
  for (unsigned i = 7; i < clock.length(); ++i) if (!isDigit(clock[i])) return false;
  int d = date.substring(0,2).toInt(), m = date.substring(2,4).toInt();
  int y = 2000 + date.substring(4,6).toInt();
  int h = clock.substring(0,2).toInt(), n = clock.substring(2,4).toInt(), s = clock.substring(4,6).toInt();
  int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (m < 1 || m > 12 || h > 23 || n > 59 || s > 59 || y < 2020) return false;
  if (y % 4 == 0) days[1] = 29;
  if (d < 1 || d > days[m-1]) return false;
  snprintf(out, 21, "%04d-%02d-%02dT%02d:%02d:%02dZ", y,m,d,h,n,s);
  secondOfDay = h * 3600 + n * 60 + s;
  return true;
}
}
void GnssProvider::start() {
  if (active) { Serial.println("GNSS attempt already running."); return; }
  valid = false; previousUtc[0] = '\0'; lastDiagnostic = ""; lastGsvSummary = ""; gsvAttempted = false; result = "starting"; power = -1;
  started = millis();
  bool ready = false;
  for (int attempt = 0; attempt < 5 && !ready; ++attempt) {
    ready = modem.powerOn();
    if (!ready) delay(1500);
  }
  if (!ready) { result = "BC20 not ready; retry gnss fix"; stop(); Serial.println(result); return; }
  uint8_t state = modem.getQGNSSC();
  if (state > 1) { result = "GNSS power state unknown"; stop(); Serial.println(result); return; }
  power = state;
  if (state != ON && !modem.setQGNSSC(ON)) { result = "GNSS start failed"; stop(); Serial.println(result); return; }
  power = -1;
  bool confirmed = false;
  for (int attempt = 0; attempt < 5 && !confirmed; ++attempt) {
    delay(1000);
    confirmed = modem.getQGNSSC() == ON;
  }
  if (!confirmed) { result = "GNSS start not confirmed"; stop(); Serial.println(result); return; }
  power = 1; active = true; nextRead = millis(); result = "acquiring";
  Serial.println("GNSS acquiring (up to 180 seconds); check outdoor antenna and sky view.");
}
void GnssProvider::stop() {
  active = false;
  if (modem.setQGNSSC(OFF)) {
    power = modem.getQGNSSC() == OFF ? 0 : -1;
  } else power = -1;
  if (power != 0) Serial.println("Warning: GNSS shutdown not confirmed; check power or disconnect USB.");
}
void GnssProvider::readGsv() {
  gsvAttempted = true;
  const bool ok = modem.getQGNSSRD(NMEA_GSV);
  lastGsvAt = millis();
  if (!ok) {
    lastGsvSummary = "GSV: unavailable (read failed or no valid sentence)";
    Serial.println(lastGsvSummary);
    return;
  }
  lastGsvSummary = String("GSV: ") + (gsvSnapshot.complete() ? "complete" : "incomplete") + " visible-by-system=";
  for (unsigned i = 0; i < gsvSnapshot.groupCount; ++i) {
    if (i) lastGsvSummary += ",";
    lastGsvSummary += String(gsvSnapshot.groups[i].system) + ":" + String(gsvSnapshot.groups[i].visible);
  }
  lastGsvSummary += " parsed-satellites=" + String(gsvSnapshot.satelliteCount);
  Serial.println(lastGsvSummary);
  for (unsigned i = 0; i < gsvSnapshot.satelliteCount; ++i) {
    const GsvSatellite &sat = gsvSnapshot.satellites[i];
    if (sat.cn0 < 0) Serial.printf("GSV satellite: %s-%u C/N0=unavailable\n", sat.system, sat.prn);
    else Serial.printf("GSV satellite: %s-%u C/N0=%d dB-Hz\n", sat.system, sat.prn, sat.cn0);
  }
}
bool GnssProvider::readFix() {
  sRMC = {}; sGGA = {}; sGSA = {};
  const bool rmcRead = modem.getQGNSSRD(NMEA_RMC);
  const bool ggaRead = modem.getQGNSSRD(NMEA_GGA);
  const bool gsaRead = modem.getQGNSSRD(NMEA_GSA);
  String rmcStatus = "unavailable", ggaQuality = "unavailable";
  String used = "unavailable", hdop = "unavailable", modes = "unavailable";
  double value;
  if (rmcRead && (sRMC.DataStatus == "A" || sRMC.DataStatus == "V")) rmcStatus = sRMC.DataStatus;
  if (ggaRead && number(sGGA.Status, value) && value >= 0 && value <= 8 && floor(value) == value) ggaQuality = sGGA.Status;
  if (ggaRead && number(sGGA.StatelliteNum, value) && value >= 0 && value <= 99 && floor(value) == value) used = sGGA.StatelliteNum;
  if (ggaRead && number(sGGA.HDOP, value) && value >= 0 && value <= 99) hdop = sGGA.HDOP;
  if (gsaRead) {
    modes = "";
    for (unsigned i = 0; i < sGSA.NUM && i < 4; ++i) {
      const String &mode = sGSA.data[i].FixStatus;
      if (mode == "1" || mode == "2" || mode == "3") {
        if (modes.length()) modes += "/";
        modes += mode;
      }
    }
    if (!modes.length()) modes = "unavailable";
  }
  lastDiagnostic = "GNSS sample: RMC=" + rmcStatus + " GGA-quality=" + ggaQuality +
      " GSA-fix=" + modes + " satellites-used=" + used + " HDOP=" + hdop +
      " reads(RMC/GGA/GSA)=" + (rmcRead ? "ok" : "failed") + "/" +
      (ggaRead ? "ok" : "failed") + "/" + (gsaRead ? "ok" : "failed");
  diagnosticAt = millis();
  Serial.println(lastDiagnostic);
  if (!rmcRead || !ggaRead || !gsaRead) return false;
  LocationFix candidate = {};
  double quality, sats, speed, ggaLat, ggaLon;
  int rmcSecond, ggaSecond;
  char ignored[21];
  if (sRMC.DataStatus != "A" || sRMC.PositioningMode == "N" ||
      !coordinate(sRMC.LatitudeVal, sRMC.LatitudeDir, true, candidate.latitude) ||
      !coordinate(sRMC.LongitudeVal, sRMC.LongitudeDir, false, candidate.longitude) ||
      !coordinate(sGGA.LatitudeVal, sGGA.LatitudeDir, true, ggaLat) ||
      !coordinate(sGGA.LongitudeVal, sGGA.LongitudeDir, false, ggaLon) ||
      fabs(candidate.latitude - ggaLat) > 0.001 || fabs(candidate.longitude - ggaLon) > 0.001 ||
      !utc(sRMC.UTC_Date, sRMC.UTC_Time, candidate.capturedAtUtc, rmcSecond) ||
      !utc(sRMC.UTC_Date, sGGA.UTC_Time, ignored, ggaSecond) ||
      !number(sGGA.Status, quality) || quality < 1 ||
      !number(sGGA.StatelliteNum, sats) || sats < 4 || sats > 99 || floor(sats) != sats ||
      !number(sGGA.HDOP, candidate.hdop) || candidate.hdop <= 0 || candidate.hdop > 2.5 ||
      !number(sGGA.Altitude, candidate.altitudeM) || !number(sRMC.GroundSpeed, speed) || speed < 0 ||
      !number(sRMC.GroundHeading, candidate.headingDeg) || candidate.headingDeg < 0 || candidate.headingDeg >= 360) return false;
  int difference = abs(rmcSecond - ggaSecond);
  if (difference > 5 && difference < 86395) return false;
  bool fix3d = false;
  for (unsigned i = 0; i < sGSA.NUM && i < 4; ++i) if (sGSA.data[i].FixStatus == "3") fix3d = true;
  if (!fix3d) return false;
  bool fresh = previousUtc[0] && strcmp(candidate.capturedAtUtc, previousUtc) > 0;
  memcpy(previousUtc, candidate.capturedAtUtc, sizeof(previousUtc));
  if (!fresh) return false;
  candidate.satellitesUsed = static_cast<unsigned>(sats);
  candidate.speedKmh = speed * 1.852;
  candidate.acquiredAt = millis(); fix = candidate; valid = true;
  return true;
}
void GnssProvider::update() {
  if (!active) return;
  if (millis() - started >= kTimeout) {
    valid = false; result = "timed out: no valid fix; check GNSS antenna and sky view";
    stop(); Serial.println(result); return;
  }
  if (static_cast<long>(millis() - nextRead) < 0) return;
  const bool found = readFix();
  if (millis() - started < kTimeout && (!gsvAttempted || millis() - lastGsvAt >= 15000)) readGsv();
  if (found && millis() - started < kTimeout) {
    result = "valid fix"; stop();
    Serial.printf("GNSS fix valid: 3D, satellites=%u, HDOP=%.2f, UTC=%s\n", fix.satellitesUsed, fix.hdop, fix.capturedAtUtc);
    Serial.printf("Position: %.6f, %.6f; altitude: %.2f m\n", fix.latitude, fix.longitude, fix.altitudeM);
  } else { valid = false; nextRead = millis() + 3000; }
}
bool GnssProvider::current(LocationFix &out) {
  if (!valid || active || millis() - fix.acquiredAt > kLifetime) return false;
  out = fix; return true;
}
void GnssProvider::status() {
  LocationFix currentFix;
  Serial.printf("GNSS power: %s; result: %s; publishable: %s\n", power == 0 ? "off" : power == 1 ? "on" : "unknown", result, current(currentFix) ? "yes" : "no");
  if (lastDiagnostic.length()) {
    Serial.printf("Last sample age: %lu s\n", (millis() - diagnosticAt) / 1000);
    Serial.println(lastDiagnostic);
  } else Serial.println("No diagnostic sample from the current attempt.");
  if (lastGsvSummary.length()) {
    Serial.printf("Last GSV sample age: %lu s\n", (millis() - lastGsvAt) / 1000);
    Serial.println(lastGsvSummary);
  } else Serial.println("No GSV sample from the current attempt.");
  if (valid) Serial.printf("Last fix: 3D, satellites=%u, HDOP=%.2f, UTC=%s, age=%lu s\n", fix.satellitesUsed, fix.hdop, fix.capturedAtUtc, (millis() - fix.acquiredAt)/1000);
}

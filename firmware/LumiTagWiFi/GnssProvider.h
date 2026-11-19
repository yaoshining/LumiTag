#pragma once
#include <Arduino.h>
#include "src/DFRobot_NBIoT/DFRobot_BC20.h"

struct LocationFix {
  double latitude, longitude, altitudeM, speedKmh, headingDeg, hdop;
  unsigned satellitesUsed;
  char capturedAtUtc[21];
  unsigned long acquiredAt;
};

class GnssProvider {
 public:
  void start();
  void update();
  void status();
  bool current(LocationFix &out);
 private:
  DFRobot_BC20 modem;
  LocationFix fix = {};
  bool active = false, valid = false;
  int power = -1;
  unsigned long started = 0, nextRead = 0;
  char previousUtc[21] = {};
  String lastDiagnostic;
  unsigned long diagnosticAt = 0;
  unsigned long lastGsvAt = 0;
  bool gsvAttempted = false;
  String lastGsvSummary;
  const char *result = "not attempted";
  void stop();
  bool readFix();
  void readGsv();
};

#pragma once
#include <stdint.h>
#include <stddef.h>

struct GsvSatellite { char system[3]; unsigned prn; int cn0; };
struct GsvGroup { char system[3]; unsigned totalParts, visible, seenMask; };
struct GsvSnapshot {
  GsvGroup groups[8]; unsigned groupCount;
  GsvSatellite satellites[60]; unsigned satelliteCount;
  bool truncated, malformed;
  GsvSnapshot();
  bool add(const char *line);
  bool complete() const;
};

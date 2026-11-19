#include "GsvParser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

GsvSnapshot::GsvSnapshot() : groupCount(0), satelliteCount(0), truncated(false), malformed(false) {
  memset(groups, 0, sizeof(groups)); memset(satellites, 0, sizeof(satellites));
}
static bool unsignedField(const char *s, unsigned &out) {
  if (!*s) return false;
  unsigned v = 0;
  for (; *s; ++s) { if (!isdigit((unsigned char)*s) || v > 999) return false; v = v * 10 + (*s - '0'); }
  out = v; return true;
}
static int hexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
bool GsvSnapshot::add(const char *line) {
  const char *start = strchr(line, '$');
  if (!start || strlen(start) < 9 || strncmp(start + 3, "GSV,", 4) != 0) return false;
  const char *star = strchr(start, '*');
  if (!star || !star[1] || !star[2]) { malformed = true; return false; }
  unsigned sum = 0; for (const char *p = start + 1; p < star; ++p) sum ^= (unsigned char)*p;
  int a = hexDigit(star[1]), b = hexDigit(star[2]);
  if (a < 0 || b < 0 || sum != (unsigned)(a * 16 + b)) { malformed = true; return false; }
  size_t len = star - start;
  if (len >= 180) { malformed = true; return false; }
  char buf[180]; memcpy(buf, start, len); buf[len] = 0;
  char *fields[24]; unsigned n = 0; fields[n++] = buf;
  for (char *p = buf; *p; ++p) if (*p == ',') { *p = 0; if (n >= 24) { malformed = true; return false; } fields[n++] = p + 1; }
  unsigned parts, part, visible;
  if (n < 4 || !unsignedField(fields[1], parts) || !unsignedField(fields[2], part) ||
      !unsignedField(fields[3], visible) || parts < 1 || parts > 16 || part < 1 || part > parts || visible > 99) { malformed = true; return false; }
  char sys[3] = {buf[1], buf[2], 0};
  unsigned g = 0; while (g < groupCount && strcmp(groups[g].system, sys)) ++g;
  if (g == groupCount) {
    if (groupCount == 8) { truncated = true; return false; }
    memcpy(groups[g].system, sys, 3); groups[g].totalParts = parts; groups[g].visible = visible; ++groupCount;
  }
  GsvGroup &group = groups[g];
  if (group.totalParts != parts || group.visible != visible || (group.seenMask & (1U << (part - 1)))) { malformed = true; return false; }
  group.seenMask |= 1U << (part - 1);
  for (unsigned i = 4; i + 3 < n && i < 20; i += 4) {
    unsigned prn, cn;
    if (!unsignedField(fields[i], prn) || prn == 0 || prn > 999) continue;
    if (satelliteCount == 60) { truncated = true; break; }
    GsvSatellite &sat = satellites[satelliteCount++]; memcpy(sat.system, sys, 3); sat.prn = prn;
    sat.cn0 = unsignedField(fields[i+3], cn) && cn <= 99 ? (int)cn : -1;
  }
  return true;
}
bool GsvSnapshot::complete() const {
  if (!groupCount || malformed || truncated) return false;
  for (unsigned i = 0; i < groupCount; ++i)
    if (groups[i].seenMask != ((1U << groups[i].totalParts) - 1)) return false;
  return true;
}

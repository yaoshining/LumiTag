#include "../LumiTagWiFi/GsvParser.h"
#include <cassert>
#include <cstdio>
#include <string>
static std::string sentence(const char *body) {
  unsigned char sum = 0; for (const char *p = body; *p; ++p) sum ^= *p;
  char suffix[5]; snprintf(suffix, sizeof(suffix), "*%02X", sum);
  return std::string("+QGNSSRD: $") + body + suffix;
}
int main() {
  GsvSnapshot s;
  assert(s.add(sentence("GPGSV,2,1,05,01,40,120,35,02,50,200,,03,30,090,20,04,15,040,00").c_str()));
  assert(!s.complete() && s.groupCount == 1 && s.groups[0].visible == 5 && s.satelliteCount == 4);
  assert(s.satellites[1].cn0 == -1 && s.satellites[3].cn0 == 0);
  assert(s.add(sentence("GPGSV,2,2,05,05,20,090,25").c_str()));
  assert(s.complete() && s.satelliteCount == 5);
  assert(s.add(sentence("GBGSV,1,1,00").c_str()));
  assert(s.complete() && s.groupCount == 2 && s.groups[1].visible == 0);
  GsvSnapshot invalid;
  assert(!invalid.add("$GPGSV,1,1,00*00"));
  assert(!invalid.complete() && invalid.malformed);
  GsvSnapshot bad; assert(!bad.add(sentence("GPGSV,2,0,99").c_str()) && bad.malformed);
  assert(!s.add(sentence("GPGSV,2,1,05,01,40,120,35").c_str()) && !s.complete());
  puts("GSV parser tests passed");
}

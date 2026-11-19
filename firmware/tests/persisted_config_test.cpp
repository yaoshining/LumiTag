#include "../LumiTagWiFi/PersistedConfig.h"
#include <assert.h>

int main() {
  PersistedConfig config = {};
  assert(!persistedConfigValid(config));
  config.version = 1;
  strcpy(config.ssid, "test-network");
  strcpy(config.product, "test-product");
  strcpy(config.device, "test-device");
  strcpy(config.secret, "test-secret");
  assert(persistedConfigValid(config));
  config.secret[0] = '\0'; assert(!persistedConfigValid(config));
  strcpy(config.secret, "test-secret");
  memset(config.password, 'x', sizeof(config.password)); assert(!persistedConfigValid(config));
  config.password[sizeof(config.password) - 1] = '\0'; assert(persistedConfigValid(config));
  config.version = 2; assert(!persistedConfigValid(config));
}

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct PersistedConfig {
  uint32_t version;
  char ssid[33];
  char password[65];
  char product[65];
  char device[65];
  char secret[129];
};

inline bool configFieldValid(const char* data, size_t capacity, bool required) {
  return memchr(data, '\0', capacity) && (!required || data[0] != '\0');
}

inline bool persistedConfigValid(const PersistedConfig& stored) {
  return stored.version == 1 && configFieldValid(stored.ssid, sizeof(stored.ssid), true) &&
      configFieldValid(stored.password, sizeof(stored.password), false) &&
      configFieldValid(stored.product, sizeof(stored.product), true) &&
      configFieldValid(stored.device, sizeof(stored.device), true) &&
      configFieldValid(stored.secret, sizeof(stored.secret), true);
}

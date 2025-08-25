#pragma once
#include <Arduino.h>

namespace storage {
void begin();
int32_t loadCalQ16(int32_t def);
void saveCalQ16(int32_t v);
int32_t loadTareRaw(int32_t def);
void saveTareRaw(int32_t v);
int32_t loadSetpointMg(int32_t def);
void saveSetpointMg(int32_t v);
}  // namespace storage

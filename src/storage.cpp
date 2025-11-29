#include "storage.h"

#include <Preferences.h>

#include "config.h"

namespace storage {
static Preferences prefs;

void begin() { prefs.begin(NVS_NAMESPACE, false); }

int32_t loadCalQ16(int32_t def) { return prefs.getInt(KEY_CAL_Q16, def); }
void saveCalQ16(int32_t v) { prefs.putInt(KEY_CAL_Q16, v); }

int32_t loadTareRaw(int32_t def) { return prefs.getInt(KEY_TARE_RAW, def); }
void saveTareRaw(int32_t v) { prefs.putInt(KEY_TARE_RAW, v); }

int32_t loadSetpointMg(int32_t def) { return prefs.getInt(KEY_SETPOINT, def); }
void saveSetpointMg(int32_t v) { prefs.putInt(KEY_SETPOINT, v); }

float loadKv(float def) { return prefs.getFloat(KEY_KV, def); }
void saveKv(float v) { prefs.putFloat(KEY_KV, v); }
}  // namespace storage

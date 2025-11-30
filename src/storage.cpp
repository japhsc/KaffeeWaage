#include "storage.h"

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"

namespace storage {
static Preferences prefs;

static void logPersist(const char* key, int32_t prev, bool hadPrev,
                       int32_t value) {
    if (!hadPrev) {
        Serial.printf("Persist %s = %ld (new)\n", key, (long)value);
    } else if (prev != value) {
        Serial.printf("Persist %s: %ld -> %ld\n", key, (long)prev,
                      (long)value);
    }
}

static void logPersist(const char* key, float prev, bool hadPrev, float value) {
    if (!hadPrev) {
        Serial.printf("Persist %s = %.4f (new)\n", key, (double)value);
    } else if (prev != value) {
        Serial.printf("Persist %s: %.4f -> %.4f\n", key, (double)prev,
                      (double)value);
    }
}

void begin() { prefs.begin(NVS_NAMESPACE, false); }

int32_t loadCalQ16(int32_t def) { return prefs.getInt(KEY_CAL_Q16, def); }
void saveCalQ16(int32_t v) {
    bool hadPrev = prefs.isKey(KEY_CAL_Q16);
    int32_t prev = prefs.getInt(KEY_CAL_Q16, v);
    prefs.putInt(KEY_CAL_Q16, v);
    logPersist(KEY_CAL_Q16, prev, hadPrev, v);
}

int32_t loadTareRaw(int32_t def) { return prefs.getInt(KEY_TARE_RAW, def); }
void saveTareRaw(int32_t v) {
    bool hadPrev = prefs.isKey(KEY_TARE_RAW);
    int32_t prev = prefs.getInt(KEY_TARE_RAW, v);
    prefs.putInt(KEY_TARE_RAW, v);
    logPersist(KEY_TARE_RAW, prev, hadPrev, v);
}

int32_t loadSetpointMg(int32_t def) { return prefs.getInt(KEY_SETPOINT, def); }
void saveSetpointMg(int32_t v) {
    bool hadPrev = prefs.isKey(KEY_SETPOINT);
    int32_t prev = prefs.getInt(KEY_SETPOINT, v);
    prefs.putInt(KEY_SETPOINT, v);
    logPersist(KEY_SETPOINT, prev, hadPrev, v);
}

float loadKv(float def) { return prefs.getFloat(KEY_KV, def); }
void saveKv(float v) {
    bool hadPrev = prefs.isKey(KEY_KV);
    float prev = prefs.getFloat(KEY_KV, v);
    prefs.putFloat(KEY_KV, v);
    logPersist(KEY_KV, prev, hadPrev, v);
}
}  // namespace storage

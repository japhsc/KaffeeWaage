#pragma once
#include <Arduino.h>

// ---------------- Pins ----------------
// HX711
constexpr uint8_t PIN_HX_DT   = 19;
constexpr uint8_t PIN_HX_SCK  = 18;
// MAX7219 (LedControl)
constexpr uint8_t PIN_MAX_DIN = 22;
constexpr uint8_t PIN_MAX_CLK = 5;
constexpr uint8_t PIN_MAX_CS  = 15;
// KY-040 encoder
constexpr uint8_t PIN_ENC_A   = 32;
constexpr uint8_t PIN_ENC_B   = 33;
constexpr uint8_t PIN_ENC_SW  = 25;     // Tare button (push)
// Start button
constexpr uint8_t PIN_BTN_START = 23;   // Active LOW (to GND)
// Relay pins (kept LOW, active HIGH)
constexpr uint8_t PIN_RELAY     = 26;
constexpr uint8_t PIN_RELAY_LED = 2;

// ---------------- Display ----------------
// If your module is wired right-to-left, keep true. Flip if your digits appear mirrored.
constexpr bool DISPLAY_RIGHT_TO_LEFT = true;
constexpr uint8_t DISPLAY_INTENSITY  = 4;   // 0..15

// ---------------- Scale & filtering ----------------
// Compile-time offsets
constexpr int32_t CUTOFF_OFFSET_MG      = 1000;     // stop early by this many mg (0.3 g => 300)
constexpr int32_t SCALE_OFFSET_COUNTS   = -326407;  // raw baseline offset (preferred)
// If you prefer mg-level offset after scaling, set above to 0 and use this instead.
constexpr int32_t SCALE_OFFSET_MG       = 0;        // added after scaling

// Calibration factor (Q16 fixed-point): mg per raw count << 16
// Example: 22000/(-155332+326407) = 0.1286 mg/count => (int32)((0.1286f * 65536.0f) + 0.5f) = 9437
constexpr int32_t CAL_MG_PER_COUNT_Q16  = (int32_t)((0.1286f * 65536.0f) + 0.5f); // <-- TUNE ME

// Filtering
constexpr uint16_t HX711_PERIOD_MS      = 100;  // ~10 SPS
constexpr uint8_t  IIR_ALPHA_DIV        = 4;    // alpha = 1/4 = 0.25

// ---------------- UX & limits ----------------
constexpr float    SETPOINT_MAX_G       = 200.0f;
constexpr int32_t  HYSTERESIS_MG        = 100;  // 0.1 g
constexpr uint32_t SHOW_SP_MS           = 2000;
constexpr uint32_t DONE_HOLD_MS         = 1500;
constexpr uint32_t MEASURE_TIMEOUT_MS   = 30000;

// Encoder acceleration (ticks per second thresholds)
constexpr float ENC_TPS_FAST  = 100.0f; // > fast => 1.0 g
constexpr float ENC_TPS_MED   = 50.0f;  // > med  => 0.5 g

// Step sizes (grams)
constexpr float ENC_STEP_SLOW_G = 0.025f;
constexpr float ENC_STEP_MED_G  = 0.1f;
constexpr float ENC_STEP_FAST_G = 0.5f;

// Debounce
constexpr uint32_t DEBOUNCE_MS          = 25;

// ---------------- Stability detection ----------------
constexpr uint8_t  STAB_WINDOW_SAMPLES = 10;   // 1.0 s at 10 Hz
constexpr int32_t  STAB_STDDEV_MG      = 30;   // 0.03g
constexpr int32_t  STAB_P2P_MG         = 100;  // 0.10 g
constexpr uint32_t STAB_DWELL_MS       = 300;  // must remain quiet for this long

// ---------------- Calibration (long-press flow) ----------------
constexpr uint32_t ENC_LONGPRESS_MS     = 1500; // hold encoder SW this long
constexpr float    CAL_SPAN_MASS_G      = 22.0f; // known weight for span step, e.g. 22g

// ---------------- Persistence (NVS) ----------------
constexpr char NVS_NAMESPACE[] = "coffee";
constexpr char KEY_CAL_Q16[]   = "cal_q16";
constexpr char KEY_TARE_RAW[]  = "tare_raw";
constexpr char KEY_SETPOINT[]  = "setpoint";

// Behavior flags
constexpr bool REQUIRE_STABLE_FOR_TARE = true;
constexpr bool REQUIRE_STABLE_FOR_CAL  = true;

// Hint timing
constexpr uint32_t HINT_HOLD_MS        = 600;

// ---------------- WiFi & FRITZ!Box AHA ----------------
// #define USE_WIFI      // comment out to disable WiFi and AHA relay control
#ifdef USE_WIFI
constexpr char WIFI_SSID[]  = "SSID";
constexpr char WIFI_PASS[]  = "PASSWORD";

constexpr char FRITZ_BASE[] = "http://fritz.box";

constexpr char FRITZ_USER[] = "username";
constexpr char FRITZ_PASS[] = "password";

// AIN of the AHA device to control
constexpr char FRITZ_AIN[]   = "AIN_0123456789";
#endif

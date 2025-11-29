# :coffee: Kaffee Waage (ESP32)

ESP32-based coffee dose scale with HX711 load cell, MAX7219 7-seg display, rotary encoder, start button, and relay (or FRITZ!Box AHA smart plug) control. Calibration, tare, setpoint, and dynamic cutoff are persisted in NVS.

## :sparkles: What you get

- Fast sampling (80 SPS capable) with velocity/accel-based cutoff for tight dosing
- On-device tare, setpoint editing, and long-press calibration (all persisted)
- MAX7219 8-digit display with stability indicator and WiFi status (optional)
- GPIO relay control or FRITZ!Box AHA smart plug via WiFi (`USE_WIFI`)

## :toolbox: Hardware overview

- ESP32 dev board
- HX711 + load cell
- MAX7219 8-digit 7-seg display
- KY-040 rotary encoder (rotate = setpoint, press = tare/calibration)
- Start push button (active LOW)
- Relay on GPIO or FRITZ!Box AHA smart plug over WiFi (`USE_WIFI`)

## :electric_plug: Wiring

### HX711

- DT  → GPIO 19
- SCK → GPIO 18
- VCC → 3V3
- GND → GND

### MAX7219 display

- DIN → GPIO 22
- CS  → GPIO 15
- CLK → GPIO 5
- VCC → 5V
- GND → GND

### Rotary encoder (KY-040)

- CLK/A → GPIO 32
- DT/B  → GPIO 33
- SW    → GPIO 25
- \+    → 3V3
- GND   → GND

### Buttons

- Start → GPIO 23 to GND (active LOW)
- Tare/Cal → encoder push SW above

### Relay / indicator

- Relay control → GPIO 26 (active HIGH)
- Relay LED / onboard LED → GPIO 2

## :zap: HX711 80 SPS mod

Most breakout boards ship the HX711 in 10 SPS mode (RATE pin tied to GND). For faster pours and better cutoff prediction, switch to 80 SPS: carefully desolder the RATE pin from GND and wire it to VDD/3V3 instead. Keep HX711 leads short and shielded; faster sampling can pick up more noise, so revisit `HX711_PERIOD_FAST_MS` / stability thresholds if needed.

## :rocket: Build & flash (PlatformIO)

Run inside `KaffeeWaage/`:

- `pio run` (build)
- `pio run -t upload` (flash)
- `pio device monitor` (serial)

## :joystick: Operating the scale

- Tare: short-press the encoder. With `REQUIRE_STABLE_FOR_TARE` the scale must be quiet; tare is saved to NVS.
- Setpoint: turn the encoder; the value is stored automatically after a short timeout. Default max is `SETPOINT_MAX_G`.
- Start/stop: press the start button. While measuring, HX711 sampling speeds up, the relay energizes, and the cutoff uses velocity/accel prediction plus hysteresis. Press again to cancel early.
- Calibration: long-press the encoder (~1.5 s). First long-press captures zero, then place a known weight (`CAL_SPAN_MASS_G`, default 22 g) and long-press again to store the new factor in NVS.
- WiFi mode: uncomment `USE_WIFI` and set credentials to drive a FRITZ!Box AHA plug instead of the GPIO relay; WiFi status is shown on the display.

## :gear: Config constants to tune (`include/config.h`)

- **Pins:** `PIN_HX_DT`, `PIN_HX_SCK`, `PIN_MAX_DIN`, `PIN_MAX_CLK`, `PIN_MAX_CS`, `PIN_ENC_A`, `PIN_ENC_B`, `PIN_ENC_SW`, `PIN_BTN_START`, `PIN_RELAY`, `PIN_RELAY_LED` — match to your wiring; start button is active LOW; relay pin is active HIGH.
- **Display:** `DISPLAY_RIGHT_TO_LEFT` flips digit order, `DISPLAY_INTENSITY` sets brightness (0–15), `DISPLAY_IDLE_MS`/`DISPLAY_MEAS_MS` throttle refresh in idle vs measuring.
- **Scale & calibration:** `SCALE_OFFSET_COUNTS` raw baseline offset, `SCALE_OFFSET_MG` optional mg offset, `CUTOFF_OFFSET_MG` legacy fixed offset, `CAL_MG_PER_COUNT_Q16` default counts→mg factor (overridden by on-device calibration), `HX711_PERIOD_IDLE_MS`/`HX711_PERIOD_FAST_MS` sampling, `NOTREADY_MULT`/`NOTREADY_MARGIN_MS` timeout detection, `HX711_STARTUP_GRACE_MS` boot grace, `IIR_ALPHA_DIV` display smoothing, `CAL_SPAN_MASS_G` reference mass for long-press calibration.
- **UX & limits:** `SETPOINT_MAX_G`, `HYSTERESIS_MG`, `SHOW_SP_MS`, `DONE_HOLD_MS`, `MEASURE_TIMEOUT_MS`, encoder thresholds `ENC_TPS_FAST`/`ENC_TPS_MED` and steps `ENC_STEP_SLOW_G`/`ENC_STEP_MED_G`/`ENC_STEP_FAST_G`, `DEBOUNCE_MS`, `REQUIRE_STABLE_FOR_TARE`, `REQUIRE_STABLE_FOR_CAL`, `HINT_HOLD_MS`.
- **Stability detection:** `STAB_WINDOW_SAMPLES`, `STAB_STDDEV_MG`, `STAB_P2P_MG`, `STAB_DWELL_MS` define when readings are considered stable.
- **Dynamic cutoff model:** `TAU_MEAS_MS`, `TAU_COMM_MS` cover measurement/plug latency; `KV_EMA_ALPHA` is the learning rate for k_v; `V_MIN_GPS` is the minimum flow used when learning; `ERROR_DISPLAY_DEBOUNCE_MS` filters brief HX711 errors.
- **Persistence keys:** `NVS_NAMESPACE`, `KEY_CAL_Q16`, `KEY_TARE_RAW`, `KEY_SETPOINT`, `KEY_KV` only need changes if you must isolate NVS data.
- **WiFi & FRITZ!Box AHA:** `USE_WIFI` enables WiFi mode; set `WIFI_SSID`/`WIFI_PASS`, `FRITZ_BASE`, `FRITZ_USER`/`FRITZ_PASS`, and `FRITZ_AIN` for your smart plug.

## :mag_right: How it works

- **Stability detection:** Samples are median-of-3 filtered, converted to mg, then smoothed with an IIR. A sliding window (`STAB_WINDOW_SAMPLES`) checks standard deviation (`STAB_STDDEV_MG`) and peak-to-peak (`STAB_P2P_MG`). Stability is declared only after it stays quiet for `STAB_DWELL_MS`, which gates tare/calibration (when required) and the steady “stable” indicator.
- **Dynamic cutoff model:** During a run the fast α–β filter estimates weight, velocity, and acceleration. The controller subtracts a predicted offset `v*tau + 0.5*a*tau^2 + k_v*v` where `tau` covers HX711 + relay/plug latency (`TAU_*`) and `k_v` is learned from past overshoot (`KV_EMA_ALPHA`, bounded by `V_MIN_GPS`). `HYSTERESIS_MG` adds a buffer so the relay releases once the predicted setpoint is reached.
- **FRITZ!Box AHA vs GPIO relay:** With `USE_WIFI` defined, the GPIO relay is replaced by WiFi control of a FRITZ!Box AHA smart plug (`FRITZ_BASE`, `FRITZ_USER`/`FRITZ_PASS`, `FRITZ_AIN`). The onboard LED pin still indicates state. Without `USE_WIFI`, the local relay pins (`PIN_RELAY`, `PIN_RELAY_LED`) drive a direct load.

## :crystal_ball: Dynamic cutoff detail and tuning

- Offset math: `offset_dyn = v*tau + 0.5*a*tau^2 + k_v*(v/1000)`, with `v`/`a` in mg/s, `tau` in s (`TAU_MEAS_MS + TAU_COMM_MS`), and `k_v` in mg per g/s. Effective target is `setpoint - offset_dyn`, then compared against `fastMg() + HYSTERESIS_MG`.
- Terms: `v*tau` predicts incoming mass during latency; `0.5*a*tau^2` compensates accel/decel; `k_v` is a learned bias for extra overshoot not explained by latency. `HYSTERESIS_MG` prevents chatter around the target.
- Learning: at stop, it captures flow in g/s (`last_v_stop_gps_`) and the final stable mass. Overshoot `eps = final - setpoint` divided by flow produces a per-flow correction; that is EMA’d into `k_v` with `KV_EMA_ALPHA` (guarded by `V_MIN_GPS`) and persisted.
- Practical tuning for premature stops (under-dosing): lower `TAU_COMM_MS` if your relay/plug is faster; clear `k_v` to zero and let it relearn; increase `HYSTERESIS_MG` slightly if noise trips early. For overshoot, do the opposite (raise tau or `k_v`, or lower hysteresis).
- Debugging: temporarily log `v`, `a`, `offset_dyn`, `effective`, `fastMg`, and the cutoff decision to confirm whether math or noise is pulling the cutoff early/late.

## :pencil: LedControl tweaks

- Kept a local copy of `LedControl.h/.cpp` so PlatformIO doesn’t fetch the stock library (see commented `lib_deps`).
- Added an ESP32-friendly `pgmspace` include guard and `#pragma once` to avoid AVR-only headers.
- Expanded the PROGMEM `charTable` beyond the stock set (which only had 0–9, A–F, H, L, P, space/dash/dot) with recognizable glyphs for: `G`, `I`, `J/j`, `O`, `R`, `S`, `U/u`, `Y/y`, `Z/z`, and lowercase `a,b,c,d,e,f,g,h,i,l,n,o,p,q,r,t` plus duplicated digits and `_ . -`. That keeps UI strings (Err/Hold/Cal/WiFi hints) readable on the 7-seg display.
- Class API remains unchanged, so it can be swapped with upstream if you prefer; only the glyph table and portability bits differ.

## :triangular_ruler: Manual calibration math (optional)

If you prefer to calculate the factor offline: temporarily log `gScale.rawCounts()` in `Scale::update()`, record `raw0` empty and `raw1` with a known mass `m` mg. Compute `CAL_MG_PER_COUNT = m / (raw1 - raw0)` and set `CAL_MG_PER_COUNT_Q16 = mg_per_count * 65536` in `config.h`. Combine with `SCALE_OFFSET_COUNTS`/`SCALE_OFFSET_MG` as needed.

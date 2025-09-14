## 🔧 Build & flash
1) Put these files into a new folder `coffee-scale-esp32/` exactly as shown.
2) Run in that folder:
   - `pio run` (build)
   - `pio run -t upload` (flash)
   - `pio device monitor` (serial)

## 📏 Calibrating `CAL_MG_PER_COUNT`
1) Temporarily `Serial.println(gScale.rawCounts());` inside `Scale::update()` to observe counts.
2) Record `raw0` with nothing on the scale; place a known mass (e.g., 200.0 g) and record `raw1`.
3) Compute: `CAL_MG_PER_COUNT = 200000 / (raw1 - raw0)` (round to nearest integer). Update `include/config.h` and rebuild.

> Offsets are compile-time constants as requested: `CUTOFF_OFFSET_MG` and `SCALE_OFFSET_COUNTS` / `SCALE_OFFSET_MG`.

## Issues:
[x] Make "stable detection" more robust

#pragma once

enum class AppState : uint8_t {
  IDLE = 0,
  SHOW_SETPOINT,
  MEASURING,
  DONE_HOLD,
  CAL_ZERO,   // long-press enters calibration: capture zero
  CAL_SPAN,   // prompt to place known mass; long-press to capture span
  ERROR_STATE
};

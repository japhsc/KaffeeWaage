#pragma once
#include <Arduino.h>
#include "state.h"
#include "scale.h"
#include "encoder.h"
#include "buttons.h"
#include "display.h"
#include "relay.h"

class Controller {
public:
  void begin(Scale* sc, Encoder* enc, Buttons* btn, Display* disp, Relay* rel);
  void update();
  void setSetpointMg(int32_t mg){ setpoint_mg_ = mg; }
  int32_t setpointMg() const { return setpoint_mg_; }
private:
  Scale* sc_ = nullptr; Encoder* enc_ = nullptr; Buttons* btn_ = nullptr; Display* disp_ = nullptr; Relay* rel_ = nullptr;
  AppState state_ = AppState::IDLE;
  int32_t setpoint_mg_ = 14000; // default 14.0 g
  uint32_t tShowUntil_ = 0, tMeasureUntil_ = 0, tDoneUntil_ = 0;
};

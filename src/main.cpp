#include <Arduino.h>
#include "config.h"
#include "scale.h"
#include "encoder.h"
#include "buttons.h"
#include "display.h"
#include "relay.h"
#include "controller.h"
#include "storage.h"

Scale      gScale;
Encoder    gEncoder;
Buttons    gButtons;
Display    gDisplay;
Relay      gRelay;
Controller gController;

void setup(){
  Serial.begin(115200);
  delay(200);

  storage::begin(); // init NVS

  // Hardware init
  gDisplay.begin(PIN_MAX_DIN, PIN_MAX_CLK, PIN_MAX_CS);
  gScale.begin(PIN_HX_DT, PIN_HX_SCK);
  gEncoder.begin(PIN_ENC_A, PIN_ENC_B, PIN_ENC_SW);
  gButtons.begin(PIN_BTN_START);
  gRelay.begin(PIN_RELAY); // stays LOW (dummy)

  // Load persisted values
  int32_t q16 = storage::loadCalQ16(CAL_MG_PER_COUNT_Q16);
  gScale.setCalMgPerCountQ16(q16);
  int32_t tareRaw = storage::loadTareRaw(0);
  gScale.setTareRaw(tareRaw);
  int32_t sp = storage::loadSetpointMg(14000);
  gController.setSetpointMg(sp);

  gController.begin(&gScale, &gEncoder, &gButtons, &gDisplay, &gRelay);

  Serial.println("Coffee Scale ready. Using persisted calibration/tare/setpoint if available.");
}

void loop(){
  gController.update();
  // Tiny idle delay to keep the loop cooperative
  delay(1);
}

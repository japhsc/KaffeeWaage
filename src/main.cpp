#include <Arduino.h>
#include "config.h"
#include "scale.h"
#include "encoder.h"
#include "buttons.h"
#include "display.h"
#include "relay.h"
#include "controller.h"

Scale      gScale;
Encoder    gEncoder;
Buttons    gButtons;
Display    gDisplay;
Relay      gRelay;
Controller gController;

void setup(){
  Serial.begin(115200);
  delay(200);

  // Hardware init
  gDisplay.begin(PIN_MAX_DIN, PIN_MAX_CLK, PIN_MAX_CS);
  gScale.begin(PIN_HX_DT, PIN_HX_SCK);
  gEncoder.begin(PIN_ENC_A, PIN_ENC_B, PIN_ENC_SW);
  gButtons.begin(PIN_BTN_START);
  gRelay.begin(PIN_RELAY); // stays LOW (dummy)

  gController.begin(&gScale, &gEncoder, &gButtons, &gDisplay, &gRelay);

  Serial.println("Coffee Scale ready. Calibrate CAL_MG_PER_COUNT_Q16 in include/config.h");
}

void loop(){
  gController.update();
  // Tiny idle delay to keep the loop cooperative
  delay(1);
}

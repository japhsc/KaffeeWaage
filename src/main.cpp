#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "scale.h"
#include "encoder.h"
#include "buttons.h"
#include "display.h"
#include "relay.h"
#include "controller.h"
#include "storage.h"
#include "FritzAHA.h"

Scale      gScale;
Encoder    gEncoder;
Buttons    gButtons;
Display    gDisplay;
Relay      gRelay;
Controller gController;

FritzAHA fritz(FRITZ_BASE, FRITZ_USER, FRITZ_PASS);

void setup(){
  Serial.begin(115200);
  delay(200);

  storage::begin(); // init NVS

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.println("\nWiFi connected!");

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

  String sid = fritz.login();
  if (sid == "") { Serial.println("FRITZ!Box login failed"); return; }
  Serial.println("SID: " + sid);

  // ---- Generic AHA (optional) ----
  // Serial.println(fritz.aha(sid, "getswitchlist"));

  if (fritz.switch_on(sid, AIN))  Serial.println("Switch ON OK");
  delay(1500);
  if (fritz.switch_off(sid, AIN)) Serial.println("Switch OFF OK");

  int st = fritz.state(sid, AIN);
  Serial.printf("State = %d\n", st); // 0/1, -1 on error
}

void loop(){
  gController.update();
  // Tiny idle delay to keep the loop cooperative
  delay(1);
}

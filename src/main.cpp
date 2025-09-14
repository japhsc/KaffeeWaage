#include <Arduino.h>
#include <WiFi.h>

#include "FritzAHA.h"
#include "buttons.h"
#include "config.h"
#include "controller.h"
#include "display.h"
#include "encoder.h"
#include "relay.h"
#include "scale.h"
#include "storage.h"
#include "switch.h"

Scale gScale;
Encoder gEncoder;
Buttons gButtons;
Display gDisplay;
Controller gController;

FritzAHA gFritz(FRITZ_BASE, FRITZ_USER, FRITZ_PASS);
SwitchWorker gWorker(gFritz);
Relay gRelay(gWorker, FRITZ_AIN);

void setup() {
    Serial.begin(115200);
    delay(200);

    storage::begin();  // init NVS

    // Hardware init
    gDisplay.begin(PIN_MAX_DIN, PIN_MAX_CLK, PIN_MAX_CS);
    gScale.begin(PIN_HX_DT, PIN_HX_SCK);
    gEncoder.begin(PIN_ENC_A, PIN_ENC_B, PIN_ENC_SW);
    gButtons.begin(PIN_BTN_START);
    gRelay.begin(PIN_RELAY);

    gDisplay.showStartup();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi");
    gDisplay.showWifiConnecting();
    int wifi_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
        delay(300);
        Serial.print(".");
        wifi_attempts++;
        gDisplay.showWifiConnecting(wifi_attempts);
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi.");
        gDisplay.showError();
        return;
    }
    Serial.println("\nWiFi connected!");
    gDisplay.showWifiConnected();

    gWorker.begin();

    // Load persisted values
    int32_t q16 = storage::loadCalQ16(CAL_MG_PER_COUNT_Q16);
    gScale.setCalMgPerCountQ16(q16);
    int32_t tareRaw = storage::loadTareRaw(0);
    gScale.setTareRaw(tareRaw);
    int32_t sp = storage::loadSetpointMg(14000);
    gController.setSetpointMg(sp);

    gController.begin(&gScale, &gEncoder, &gButtons, &gDisplay, &gRelay);

    Serial.println(
        "Coffee Scale ready. Using persisted calibration/tare/setpoint if "
        "available.");
}

void loop() {
    gController.update();
    // Tiny idle delay to keep the loop cooperative
    delay(1);
}

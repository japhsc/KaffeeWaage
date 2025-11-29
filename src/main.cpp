#include <Arduino.h>

#include "config.h"

#ifdef USE_WIFI
#include <WiFi.h>

#include "FritzAHA.h"
#include "wrelay.h"
#else
#include "relay.h"
#endif

#include "buttons.h"
#include "controller.h"
#include "display.h"
#include "encoder.h"
#include "scale.h"
#include "storage.h"
#include "switch.h"

Scale gScale;
Encoder gEncoder;
Buttons gButtons(ENC_LONGPRESS_MS);
Display gDisplay;
Controller gController;

#ifdef USE_WIFI
FritzAHA gFritz(FRITZ_BASE, FRITZ_USER, FRITZ_PASS);
SwitchWorker gWorker(gFritz);
WifiRelay gRelay(gWorker, FRITZ_AIN);
#else
Relay gRelay;
#endif

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;  // wait for serial port to connect. Needed for native USB ports
    }
    Serial.println("Booting Coffee Scale...");

    // NVS init
    storage::begin();

    // Display init
    gDisplay.begin(PIN_MAX_DIN, PIN_MAX_CLK, PIN_MAX_CS);
    gDisplay.showStartup();

    // HID init
    gButtons.begin(PIN_BTN_START);
    gEncoder.begin(PIN_ENC_A, PIN_ENC_B, PIN_ENC_SW);

    // Scale init
    gScale.begin(PIN_HX_DT, PIN_HX_SCK);

#ifndef USE_WIFI
    // Simple relay init with optional LED indicator
    gRelay.begin(PIN_RELAY, PIN_RELAY_LED);
#else
    // WiFi relay init with LED indicator
    gRelay.begin(PIN_RELAY_LED);

    // WiFi init
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
#endif

    // Load persisted values
    int32_t q16 = storage::loadCalQ16(CAL_MG_PER_COUNT_Q16);
    gScale.setCalMgPerCountQ16(q16);
    int32_t tareRaw = storage::loadTareRaw(0);
    gScale.setTareRaw(tareRaw);
    int32_t sp = storage::loadSetpointMg(14000);
    gController.setSetpointMg(sp);
    float kv = storage::loadKv(0.0f);
    gController.setKvMgPerGps(kv);

    gController.begin(&gScale, &gEncoder, &gButtons, &gDisplay, &gRelay);

    Serial.println("Coffee Scale ready.");
    Serial.println("Using persisted calibration/tare/setpoint if available.");
    Serial.println("HX711 samples at:");
    Serial.printf("\t%u SPS during measuring.\n", lroundf(1000.0f/HX711_PERIOD_FAST_MS));
    Serial.printf("\t%u SPS during idle time.\n", lroundf(1000.0f/HX711_PERIOD_IDLE_MS));
    Serial.println("Dynamic cutoff enabled.");

    // Log persisted parameters for debugging/verification
    Serial.println("Persisted parameters:");
    Serial.print("  cal_q16: ");
    Serial.println(q16);
    Serial.print("  tare_raw: ");
    Serial.println(tareRaw);
    Serial.print("  setpoint_mg: ");
    Serial.println(sp);
    Serial.print("  k_v (mg per g/s): ");
    Serial.println(kv);
}

void loop() {
    gController.update();

    // Tiny idle delay to keep the loop cooperative
    delay(1);
}

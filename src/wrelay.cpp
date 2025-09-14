#include "wrelay.h"

void WifiRelay::begin(uint8_t pin) {
    Relay::begin(pin);
}

void WifiRelay::set(bool on) {
    Relay::set(on);

    _worker.toggle(ain_, on);
}

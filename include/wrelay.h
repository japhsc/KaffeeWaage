#pragma once
#include <Arduino.h>

#include "relay.h"
#include "switch.h"


class WifiRelay : public Relay {
    public:
     explicit WifiRelay(SwitchWorker& client, const char* ain)
          : ain_(ain), _worker(client) {}

     void begin(uint8_t pin);
     void set(bool on);

    private:
     const char* ain_;
     SwitchWorker& _worker;
};

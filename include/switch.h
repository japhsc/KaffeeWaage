#pragma once

#include "FritzAHA.h"

// ===== Class: async ON/OFF switcher backed by a FreeRTOS task =====
class SwitchWorker {
   public:
    // Public attributes (read-only outside): current SID, queue depth
    String sid;
    size_t queueDepth() const;

    // Construct with a reference to your FritzAHA client
    explicit SwitchWorker(FritzAHA& client) : _fritz(client) {}

    // Initialize: connect to FRITZ!Box and start task
    bool begin();

    // Enqueue ON/OFF by AIN
    bool on(const char* ain);
    bool off(const char* ain);
    bool toggle(const char* ain, bool state);

    // Optional: stop task and free queue
    void end();

   private:
    FritzAHA& _fritz;              // reference to AHA client
    QueueHandle_t _q = nullptr;    // command queue
    TaskHandle_t _task = nullptr;  // worker task handle

    enum Cmd : uint8_t { CMD_ON, CMD_OFF };
    struct Request {
        Cmd cmd;
        char ain[24];
    };

    bool enqueue(Cmd c, const char* ain);

    // Task body
    void taskLoop();
    static void _taskThunk(void* self);
};

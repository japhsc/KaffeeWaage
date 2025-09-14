#include "switch.h"

size_t SwitchWorker::queueDepth() const { return uxQueueMessagesWaiting(_q); }

// Initialize: start task
bool SwitchWorker::begin() {
    _q = xQueueCreate(12, sizeof(Request));
    if (!_q) return false;
    BaseType_t ok = xTaskCreatePinnedToCore(_taskThunk, "SwitchWorker", 8192,
                                            this, 1, &_task, 1);
    return ok == pdPASS;
}

// Enqueue ON/OFF by AIN
bool SwitchWorker::on(const char* ain) { return enqueue(CMD_ON, ain); }

bool SwitchWorker::off(const char* ain) { return enqueue(CMD_OFF, ain); }

bool SwitchWorker::toggle(const char* ain, bool state) {
    return enqueue(state ? CMD_ON : CMD_OFF, ain);
}

// Optional: stop task and free queue
void SwitchWorker::end() {
    if (_task) {
        vTaskDelete(_task);
        _task = nullptr;
    }
    if (_q) {
        vQueueDelete(_q);
        _q = nullptr;
    }
}

bool SwitchWorker::enqueue(Cmd c, const char* ain) {
    if (!_q) return false;
    Request r{c, {0}};
    strlcpy(r.ain, ain, sizeof(r.ain));
    return xQueueSend(_q, &r, pdMS_TO_TICKS(10)) == pdTRUE;
}

// Task body
void SwitchWorker::taskLoop() {
    // Init FritzAHA client
    sid = _fritz.login();
    if (!sid.length()) return;

    Request r;
    for (;;) {
        if (xQueueReceive(_q, &r, portMAX_DELAY) == pdTRUE) {
            bool ok = (r.cmd == CMD_ON) ? _fritz.switch_on(sid, r.ain)
                                        : _fritz.switch_off(sid, r.ain);
            Serial.printf("%s %s\n", (r.cmd == CMD_ON ? "ON " : "OFF"),
                          ok ? "ok" : "fail");
        }
    }
}

void SwitchWorker::_taskThunk(void* self) {
    static_cast<SwitchWorker*>(self)->taskLoop();
}

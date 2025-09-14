#include "display.h"

#include "config.h"

void Display::begin(uint8_t din, uint8_t clk, uint8_t cs) {
    lc_ = LedControl(din, clk, cs, 1);
    lc_.shutdown(0, false);
    lc_.setIntensity(0, DISPLAY_INTENSITY);
    lc_.clearDisplay(0);
}

int Display::mapDigit(int d) const {
    return DISPLAY_RIGHT_TO_LEFT ? d : (7 - d);
}

void Display::putChar(int pos, char c, bool dp) {
    lc_.setChar(0, mapDigit(pos), c, dp);
}
void Display::putDigit(int pos, uint8_t d, bool dp) {
    lc_.setChar(0, mapDigit(pos), char('0' + (d % 10)), dp);
}

void Display::clear() {
    for (int i = 0; i < 8; i++) putChar(i, ' ');
}

void Display::renderNumberMg(int32_t mg) {
    // Right-aligned, one decimal (##.#), negatives allowed.
    bool neg = (mg < 0);
    if (neg) mg = -mg;
    // Round to 0.1 g
    int32_t d10 = (mg + 50) / 100;  // tenths of grams
    int pos = 0;                    // rightmost digit index
    // tenths
    putDigit(pos++, d10 % 10, false);
    d10 /= 10;
    // ones with decimal point
    putDigit(pos, d10 % 10, true);
    pos++;
    d10 /= 10;
    // higher digits
    while (d10 > 0 && pos < 8) {
        putDigit(pos++, d10 % 10, false);
        d10 /= 10;
    }
    if (neg && pos < 8) putChar(pos, '-');
}

void Display::showWeightMg(int32_t mg, bool stable) {
    clear();
    renderNumberMg(mg);
    // Use leftmost DP as stability indicator
    putChar(7, ' ', stable);

    // Serial.printf("Display: %s %d mg\n", stable ? "stable" : "unstable", mg);
}

void Display::showSetpointMg(int32_t mg) {
    clear();
    putChar(7, 'S');
    putChar(6, 'P');
    renderNumberMg(mg);
}

void Display::showError() {
    clear();
    putChar(7, 'E');
    putChar(6, 'r');
    putChar(5, 'r');
}

void Display::showCalZero() {
    clear();
    putChar(7, 'C');
    putChar(6, 'A');
    putChar(5, 'L');
    putChar(4, '0');
}
void Display::showCalSpan() {
    clear();
    putChar(7, 'S');
    putChar(6, 'P');
    putChar(5, 'A');
    putChar(4, 'n');
}
void Display::showCalDone() {
    clear();
    putChar(7, 'd');
    putChar(6, 'o');
    putChar(5, 'n');
    putChar(4, 'E');
}
void Display::showHintHold() {
    clear();
    putChar(7, 'H');
    putChar(6, 'o');
    putChar(5, 'L');
    putChar(4, 'd');
}
void Display::showStartup() {
    clear();
    putChar(7, 'C');
    putChar(6, 'o');
    putChar(5, 'f');
    putChar(4, 'f');
    putChar(3, 'e');
    putChar(2, 'e');
}
void Display::showWifiConnecting(const int attempt) {
    clear();
    putChar(7, 'A');
    putChar(6, 'i');
    putChar(5, 'r');
    putChar(4, ' ');
    for (int i = 0; i < attempt && i < 4; i++)
        putChar(3 - i, '.');
}
void Display::showWifiConnected() {
    clear();
    putChar(7, 'C');
    putChar(6, 'o');
    putChar(5, 'n');
    putChar(4, 'n');
    putChar(3, 'E');
    putChar(2, 'C');
    putChar(1, 't');
    putChar(0, 'd');
}

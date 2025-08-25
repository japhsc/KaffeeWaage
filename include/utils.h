#pragma once
#include <Arduino.h>

inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
inline int32_t lround_mg(float grams) {
    return (int32_t)lroundf(grams * 1000.0f);
}
inline int32_t med3(int32_t a, int32_t b, int32_t c) {
    if (a > b) std::swap(a, b);
    if (b > c) std::swap(b, c);
    if (a > b) std::swap(a, b);
    return b;
}

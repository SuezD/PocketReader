#pragma once

#include <Arduino.h>

void drawHeader(
    const char* title,
    uint8_t batteryPercent
);

void redrawHeaderStatus(uint8_t batteryPercent);

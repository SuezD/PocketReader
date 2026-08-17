#pragma once

#include <GxEPD2_BW.h>

using EpaperDriver = GxEPD2_420_GDEY042T81;

#if defined(ARDUINO_ARCH_ESP32)
using DisplayType =
    GxEPD2_BW<EpaperDriver, EpaperDriver::HEIGHT>;
#else
using DisplayType =
    GxEPD2_BW<EpaperDriver, EpaperDriver::HEIGHT / 16>;
#endif

extern DisplayType display;

void initDisplay();

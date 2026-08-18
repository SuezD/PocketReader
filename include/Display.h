#pragma once

#include <GxEPD2_BW.h>

using EpaperDriver = GxEPD2_420_GDEY042T81;

using DisplayType =
    GxEPD2_BW<EpaperDriver, EpaperDriver::HEIGHT>;

extern DisplayType display;

void initDisplay();

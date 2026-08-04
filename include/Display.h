#pragma once

#include <GxEPD2_BW.h>

using EpaperDriver = GxEPD2_420_GDEY042T81;
using DisplayType =
    GxEPD2_BW<EpaperDriver, EpaperDriver::HEIGHT / 16>;

extern DisplayType display;

void initDisplay();

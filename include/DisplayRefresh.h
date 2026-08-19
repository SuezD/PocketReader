#pragma once

#include <Arduino.h>

void setFullRefreshWindow();
void setPartialRefreshWindow(int16_t x, int16_t y, int16_t width, int16_t height);
void setPageRefreshWindow();
bool isFullRefreshDue();

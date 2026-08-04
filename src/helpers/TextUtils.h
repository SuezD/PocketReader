#pragma once

#include <Arduino.h>
#include <gfxfont.h>

uint16_t getTextWidth(
    const char* text,
    const GFXfont* font
);

void truncateToWidth(
    const char* source,
    char* output,
    size_t outputSize,
    uint16_t maximumWidth,
    const GFXfont* font
);

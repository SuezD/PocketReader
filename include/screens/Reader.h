#pragma once

#include <Arduino.h>

#include "books/BookCache.h"

void openReader(
    const CachedBook* book,
    const ReaderDocument& document,
    uint16_t savedPage
);

bool moveReaderPreviousPage();
bool moveReaderNextPage();
void requestReaderFullRefresh();
void drawReader(uint8_t batteryPercent);

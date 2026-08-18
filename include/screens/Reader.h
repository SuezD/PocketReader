#pragma once

#include <Arduino.h>

#include "books/BookCache.h"

void openReader(
    const CachedBook* book,
    const ReaderDocument& document
);

bool moveReaderPreviousPage();
bool moveReaderNextPage();
void drawReader(uint8_t batteryPercent);

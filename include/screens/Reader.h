#pragma once

#include <Arduino.h>

#include "books/BookCache.h"

enum class ReaderEmptyOption : uint8_t
{
    MyBooks,
    AddBooks
};

void openReader(
    const CachedBook* book,
    const ReaderDocument& document,
    uint16_t savedPage
);

bool moveReaderPreviousPage();
bool moveReaderNextPage();
bool readerHasOpenDocument();
bool moveReaderEmptySelectionPrevious();
bool moveReaderEmptySelectionNext();
ReaderEmptyOption getSelectedReaderEmptyOption();
void requestReaderFullRefresh();
void drawReader(uint8_t batteryPercent);

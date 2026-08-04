#pragma once

#include "books/BookCache.h"

void drawMyBooks(uint8_t batteryPercent);
bool moveMyBooksUp();
bool moveMyBooksDown();
void redrawMyBooksSelection(uint8_t previousIndex);
uint8_t getSelectedMyBookIndex();
const CachedBook& getSelectedMyBook();

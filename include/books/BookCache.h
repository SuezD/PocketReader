#pragma once

#include <Arduino.h>

struct CachedBook
{
    const char* id;
    const char* title;
};

uint8_t getCachedBookCount();
const char* const* getCachedBookTitles();
const CachedBook& getCachedBook(uint8_t index);
const char* readCachedBookText(const CachedBook& book);

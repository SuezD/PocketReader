#pragma once

#include <Arduino.h>

struct CachedBook
{
    const char* id;
    const char* title;
};

#if defined(ARDUINO_ARCH_ESP32)
bool initBookCache();
#endif

uint8_t getCachedBookCount();
const char* const* getCachedBookTitles();
const CachedBook& getCachedBook(uint8_t index);
const char* readCachedBookText(const CachedBook& book);

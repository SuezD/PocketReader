#pragma once

#include <Arduino.h>

struct CachedBook
{
    const char* id;
    const char* title;
};

struct ReaderDocument
{
    uint32_t byteLength;
    const void* sourceContext;
    char (*readCharacter)(const void* sourceContext, uint32_t position);
};

#if defined(ARDUINO_ARCH_ESP32)
bool initBookCache();
#endif

uint8_t getCachedBookCount();
const char* const* getCachedBookTitles();
const CachedBook& getCachedBook(uint8_t index);
bool openCachedBookDocument(
    const CachedBook& book,
    ReaderDocument& document
);

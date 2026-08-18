#pragma once

#include <Arduino.h>

struct CachedBook
{
    const char* id;
    const char* title;
    const char* filePath;
};

struct ReaderDocument
{
    uint32_t byteLength;
    const void* sourceContext;
    char (*readCharacter)(const void* sourceContext, uint32_t position);
};

bool initBookCache();

uint8_t getCachedBookCount();
const char* const* getCachedBookTitles();
const CachedBook& getCachedBook(uint8_t index);
uint16_t getCachedBookPage(const CachedBook& book);
void saveCachedBookPage(const CachedBook& book, uint16_t page);
bool openCachedBookDocument(
    const CachedBook& book,
    ReaderDocument& document
);

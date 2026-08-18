#pragma once

#include <Arduino.h>
#include <FS.h>

struct CachedBook
{
    const char* id;
    const char* title;
    const char* filePath;
};

class ReaderDocument
{
public:
    ReaderDocument() = default;
    ~ReaderDocument();

    ReaderDocument(const ReaderDocument&) = delete;
    ReaderDocument& operator=(const ReaderDocument&) = delete;

    bool open(const CachedBook& book);
    void close();
    bool isOpen() const;
    uint32_t length() const;
    char readCharacter(uint32_t position) const;

private:
    static constexpr size_t READ_BUFFER_SIZE = 96;

    mutable File file;
    uint32_t byteLength = 0;
    mutable uint32_t bufferedByteStart = 0;
    mutable size_t bufferedByteCount = 0;
    mutable char readBuffer[READ_BUFFER_SIZE] = {};
};

bool initBookCache();

uint8_t getCachedBookCount();
const char* const* getCachedBookTitles();
const CachedBook& getCachedBook(uint8_t index);
uint16_t getCachedBookPage(const CachedBook& book);
void saveCachedBookPage(const CachedBook& book, uint16_t page);

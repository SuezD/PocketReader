#pragma once

#include <Arduino.h>
#include <FS.h>

#include "books/TextDocument.h"

struct CachedBook
{
    const char* id;
    const char* title;
    const char* filePath;
};

enum class BookInstallResult : uint8_t
{
    Success,
    AlreadyCached,
    LibraryFull,
    InvalidBook,
    MissingDownload,
    StorageError,
    ManifestError
};

enum class BookDeleteResult : uint8_t
{
    Success,
    NotFound,
    StorageError,
    ManifestError,
    ProgressError
};

class ReaderDocument : public TextDocument
{
public:
    ReaderDocument() = default;
    ~ReaderDocument();

    ReaderDocument(const ReaderDocument&) = delete;
    ReaderDocument& operator=(const ReaderDocument&) = delete;

    bool open(const CachedBook& book);
    void close();
    bool isOpen() const override;
    uint32_t length() const override;
    char readCharacter(uint32_t position) const override;

private:
    static constexpr size_t READ_BUFFER_SIZE = 96;

    mutable File file;
    uint32_t byteLength = 0;
    mutable uint32_t bufferedByteStart = 0;
    mutable size_t bufferedByteCount = 0;
    mutable char readBuffer[READ_BUFFER_SIZE] = {};
};

bool initBookCache();
BookInstallResult installCachedBook(
    const char* id,
    const char* title,
    const char* temporaryPath
);
const char* getBookInstallResultText(BookInstallResult result);
BookDeleteResult deleteCachedBook(const char* id);
const char* getBookDeleteResultText(BookDeleteResult result);

uint8_t getCachedBookCount();
const char* const* getCachedBookTitles();
const CachedBook& getCachedBook(uint8_t index);
const CachedBook* findCachedBook(const char* id);
size_t getAvailableBookStorageBytes();
size_t getCachedBookSizeBytes(const CachedBook& book);
uint16_t getCachedBookPage(const CachedBook& book);
void saveCachedBookPage(const CachedBook& book, uint16_t page);
const CachedBook* getLastOpenedCachedBook();
void setLastOpenedCachedBook(const CachedBook& book);
bool flushCachedBookProgress();

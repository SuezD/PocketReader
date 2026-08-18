#pragma once

#include <Arduino.h>

struct RemoteBook
{
    const char* id;
    const char* title;
    const char* downloadUrl;
};

enum class BookSyncResult : uint8_t
{
    Success,
    NotConfigured,
    NotConnected,
    TlsConfigurationMissing,
    RequestFailed,
    HttpError,
    ManifestTooLarge,
    InvalidManifest
};

class BookSync
{
public:
    BookSyncResult fetchManifest();
    uint8_t getBookCount() const;
    const RemoteBook& getBook(uint8_t index) const;
    int getHttpStatus() const;

private:
    static constexpr uint8_t MAX_BOOK_COUNT = 16;
    static constexpr size_t MAX_ID_LENGTH = 31;
    static constexpr size_t MAX_TITLE_LENGTH = 63;
    static constexpr size_t MAX_URL_LENGTH = 255;
    static constexpr size_t MAX_MANIFEST_BYTES = 12288;
    static constexpr size_t MAX_LINE_LENGTH =
        MAX_ID_LENGTH + MAX_TITLE_LENGTH + MAX_URL_LENGTH + 3;

    RemoteBook books[MAX_BOOK_COUNT] = {};
    char ids[MAX_BOOK_COUNT][MAX_ID_LENGTH + 1] = {};
    char titles[MAX_BOOK_COUNT][MAX_TITLE_LENGTH + 1] = {};
    char urls[MAX_BOOK_COUNT][MAX_URL_LENGTH + 1] = {};
    uint8_t bookCount = 0;
    int httpStatus = 0;

    void clear();
    bool parseManifest(const String& manifest);
    bool addBook(char* line);
};

BookSync& getBookSync();
const char* getBookSyncResultText(BookSyncResult result);

#include "books/BookCache.h"

#include <stdlib.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <LittleFS.h>
#endif

namespace
{
#if defined(ARDUINO_ARCH_ESP32)
    constexpr char BOOK_MANIFEST_PATH[] = "/books.tsv";
    constexpr char READING_PROGRESS_PATH[] = "/reading-progress.txt";
    constexpr size_t FILE_READ_BUFFER_SIZE = 96;
    constexpr uint8_t MAX_BOOK_COUNT = 16;
    constexpr size_t MAX_BOOK_ID_LENGTH = 31;
    constexpr size_t MAX_BOOK_TITLE_LENGTH = 63;
    constexpr size_t MAX_BOOK_PATH_LENGTH = 63;
    constexpr size_t MAX_MANIFEST_LINE_LENGTH =
        MAX_BOOK_ID_LENGTH + MAX_BOOK_TITLE_LENGTH +
        MAX_BOOK_PATH_LENGTH + 3;

    CachedBook books[MAX_BOOK_COUNT] = {};
    const char* bookTitles[MAX_BOOK_COUNT] = {};
    char bookIds[MAX_BOOK_COUNT][MAX_BOOK_ID_LENGTH + 1] = {};
    char bookTitleStorage[MAX_BOOK_COUNT][MAX_BOOK_TITLE_LENGTH + 1] = {};
    char bookPaths[MAX_BOOK_COUNT][MAX_BOOK_PATH_LENGTH + 1] = {};
    uint16_t savedPages[MAX_BOOK_COUNT] = {};
    uint8_t bookCount = 0;

    bool fileSystemReady = false;
    File currentTextFile;
    const char* currentFilePath = nullptr;
    uint32_t bufferedByteStart = 0;
    size_t bufferedByteCount = 0;
    char fileReadBuffer[FILE_READ_BUFFER_SIZE];

    bool copyManifestField(
        char* destination,
        size_t destinationSize,
        const char* source
    ) {
        const size_t length = strlen(source);

        if (length == 0 || length >= destinationSize)
        {
            return false;
        }

        memcpy(destination, source, length + 1);
        return true;
    }

    bool addManifestBook(char* line)
    {
        char* firstSeparator = strchr(line, '\t');

        if (firstSeparator == nullptr)
        {
            return false;
        }

        *firstSeparator = '\0';
        char* title = firstSeparator + 1;
        char* secondSeparator = strchr(title, '\t');

        if (secondSeparator == nullptr)
        {
            return false;
        }

        *secondSeparator = '\0';
        char* filePath = secondSeparator + 1;

        if (
            bookCount >= MAX_BOOK_COUNT ||
            filePath[0] != '/' ||
            !copyManifestField(
                bookIds[bookCount],
                sizeof(bookIds[bookCount]),
                line
            ) ||
            !copyManifestField(
                bookTitleStorage[bookCount],
                sizeof(bookTitleStorage[bookCount]),
                title
            ) ||
            !copyManifestField(
                bookPaths[bookCount],
                sizeof(bookPaths[bookCount]),
                filePath
            )
        ) {
            return false;
        }

        for (uint8_t index = 0; index < bookCount; index++)
        {
            if (strcmp(bookIds[index], bookIds[bookCount]) == 0)
            {
                return false;
            }
        }

        books[bookCount] = {
            bookIds[bookCount],
            bookTitleStorage[bookCount],
            bookPaths[bookCount]
        };
        bookTitles[bookCount] = books[bookCount].title;
        bookCount++;
        return true;
    }

    void loadBookManifest()
    {
        bookCount = 0;
        memset(savedPages, 0, sizeof(savedPages));
        File manifestFile = LittleFS.open(BOOK_MANIFEST_PATH, "r");

        if (!manifestFile)
        {
            Serial.println(F("Missing /books.tsv"));
            return;
        }

        char line[MAX_MANIFEST_LINE_LENGTH + 1];

        while (manifestFile.available() && bookCount < MAX_BOOK_COUNT)
        {
            const size_t length = manifestFile.readBytesUntil(
                '\n',
                line,
                sizeof(line) - 1
            );
            line[length] = '\0';

            if (length > 0 && line[length - 1] == '\r')
            {
                line[length - 1] = '\0';
            }

            if (line[0] != '\0' && !addManifestBook(line))
            {
                Serial.println(F("Ignoring invalid book manifest entry"));
            }
        }

        manifestFile.close();
    }

    char readLittleFsCharacter(
        const void* sourceContext,
        uint32_t position
    ) {
        if (
            sourceContext != currentFilePath ||
            !currentTextFile ||
            position >= currentTextFile.size()
        ) {
            return '\0';
        }

        const uint32_t bufferedByteEnd =
            bufferedByteStart + bufferedByteCount;

        if (
            position < bufferedByteStart ||
            position >= bufferedByteEnd
        ) {
            if (!currentTextFile.seek(position))
            {
                return '\0';
            }

            bufferedByteStart = position;
            bufferedByteCount = currentTextFile.readBytes(
                fileReadBuffer,
                FILE_READ_BUFFER_SIZE
            );
        }

        return fileReadBuffer[position - bufferedByteStart];
    }
#else
    const CachedBook BOOKS[] = {
        { "book-1", "Lorem Ipsum", nullptr }
    };

    const char* const BOOK_TITLES[] = {
        BOOKS[0].title
    };

    const char* const BOOK_TEXTS[] = {
        "Then the hobbit slipped on his ring, "
    };

    char readStringCharacter(
        const void* sourceContext,
        uint32_t position
    ) {
        return static_cast<const char*>(sourceContext)[position];
    }
#endif

#if !defined(ARDUINO_ARCH_ESP32)
    constexpr uint8_t BOOK_COUNT = sizeof(BOOKS) / sizeof(BOOKS[0]);
    uint16_t savedPages[BOOK_COUNT] = {};
#endif

    int getBookIndex(const CachedBook& book)
    {
        const uint8_t count = getCachedBookCount();

        for (uint8_t index = 0; index < count; index++)
        {
            if (strcmp(book.id, getCachedBook(index).id) == 0)
            {
                return index;
            }
        }

        return -1;
    }

#if defined(ARDUINO_ARCH_ESP32)
    void loadReadingProgress()
    {
        File progressFile = LittleFS.open(READING_PROGRESS_PATH, "r");

        if (!progressFile)
        {
            return;
        }

        char entry[64];

        while (progressFile.available())
        {
            const size_t entryLength = progressFile.readBytesUntil(
                '\n',
                entry,
                sizeof(entry) - 1
            );
            entry[entryLength] = '\0';

            char* separator = strchr(entry, '\t');

            if (separator == nullptr)
            {
                continue;
            }

            *separator = '\0';
            const uint32_t page = strtoul(separator + 1, nullptr, 10);

            if (page > UINT16_MAX)
            {
                continue;
            }

            for (uint8_t index = 0; index < bookCount; index++)
            {
                if (strcmp(entry, books[index].id) == 0)
                {
                    savedPages[index] = page;
                    break;
                }
            }
        }

        progressFile.close();
    }

    void writeReadingProgress()
    {
        if (!fileSystemReady)
        {
            return;
        }

        File progressFile = LittleFS.open(READING_PROGRESS_PATH, "w");

        if (!progressFile)
        {
            Serial.println(F("Could not save reading progress"));
            return;
        }

        for (uint8_t index = 0; index < bookCount; index++)
        {
            progressFile.print(books[index].id);
            progressFile.print('\t');
            progressFile.println(savedPages[index]);
        }

        progressFile.close();
    }
#endif
}

#if defined(ARDUINO_ARCH_ESP32)
bool initBookCache()
{
    fileSystemReady = LittleFS.begin(false);

    if (!fileSystemReady)
    {
        Serial.println(F("LittleFS unavailable"));
        return false;
    }

    loadBookManifest();
    loadReadingProgress();
    return true;
}
#endif

uint8_t getCachedBookCount()
{
#if defined(ARDUINO_ARCH_ESP32)
    return bookCount;
#else
    return BOOK_COUNT;
#endif
}

const char* const* getCachedBookTitles()
{
#if defined(ARDUINO_ARCH_ESP32)
    return bookTitles;
#else
    return BOOK_TITLES;
#endif
}

const CachedBook& getCachedBook(uint8_t index)
{
#if defined(ARDUINO_ARCH_ESP32)
    static const CachedBook EMPTY_BOOK = { "", "", "" };
    return index < bookCount ? books[index] : EMPTY_BOOK;
#else
    return BOOKS[index < BOOK_COUNT ? index : 0];
#endif
}

uint16_t getCachedBookPage(const CachedBook& book)
{
    const int index = getBookIndex(book);
    return index >= 0 ? savedPages[index] : 0;
}

void saveCachedBookPage(const CachedBook& book, uint16_t page)
{
    const int index = getBookIndex(book);

    if (index < 0 || savedPages[index] == page)
    {
        return;
    }

    savedPages[index] = page;

#if defined(ARDUINO_ARCH_ESP32)
    writeReadingProgress();
#endif
}

bool openCachedBookDocument(
    const CachedBook& book,
    ReaderDocument& document
)
{
    document = {};

#if defined(ARDUINO_ARCH_ESP32)
    if (!fileSystemReady || book.filePath == nullptr || book.filePath[0] == '\0')
    {
        return false;
    }

    currentTextFile.close();
    currentTextFile = LittleFS.open(book.filePath, "r");
    currentFilePath = nullptr;
    bufferedByteCount = 0;

    if (!currentTextFile)
    {
        Serial.print(F("Missing book file: "));
        Serial.println(book.filePath);
        return false;
    }

    if (currentTextFile.size() == 0)
    {
        Serial.print(F("Book file is empty: "));
        Serial.println(book.filePath);
        currentTextFile.close();
        return false;
    }

    currentFilePath = book.filePath;
    document.byteLength = currentTextFile.size();
    document.sourceContext = currentFilePath;
    document.readCharacter = readLittleFsCharacter;
    return true;
#else
    for (uint8_t index = 0; index < BOOK_COUNT; index++)
    {
        if (strcmp(book.id, BOOKS[index].id) == 0)
        {
            document.byteLength = strlen(BOOK_TEXTS[index]);
            document.sourceContext = BOOK_TEXTS[index];
            document.readCharacter = readStringCharacter;
            return document.byteLength > 0;
        }
    }

    return false;
#endif
}

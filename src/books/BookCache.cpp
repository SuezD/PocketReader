#include "books/BookCache.h"

#include <stdlib.h>
#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <LittleFS.h>
#endif

namespace
{
#if defined(ARDUINO_ARCH_ESP32)
    constexpr char TEST_BOOK_ID[] = "lorem-ipsum";
    constexpr char TEST_BOOK_PATH[] = "/lorem.txt";
    constexpr char READING_PROGRESS_PATH[] = "/reading-progress.txt";
    constexpr size_t FILE_READ_BUFFER_SIZE = 96;

    const CachedBook BOOKS[] = {
        { TEST_BOOK_ID, "Lorem Ipsum" }
    };

    const char* const BOOK_TITLES[] = {
        BOOKS[0].title
    };

    bool fileSystemReady = false;
    File currentTextFile;
    const char* currentFilePath = nullptr;
    uint32_t bufferedByteStart = 0;
    size_t bufferedByteCount = 0;
    char fileReadBuffer[FILE_READ_BUFFER_SIZE];

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
        { "book-1", "Lorem Ipsum" }
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

    constexpr uint8_t BOOK_COUNT =
        sizeof(BOOKS) / sizeof(BOOKS[0]);
    uint16_t savedPages[BOOK_COUNT] = {};

    int getBookIndex(const CachedBook& book)
    {
        for (uint8_t index = 0; index < BOOK_COUNT; index++)
        {
            if (strcmp(book.id, BOOKS[index].id) == 0)
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

            for (uint8_t index = 0; index < BOOK_COUNT; index++)
            {
                if (strcmp(entry, BOOKS[index].id) == 0)
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

        for (uint8_t index = 0; index < BOOK_COUNT; index++)
        {
            progressFile.print(BOOKS[index].id);
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

    loadReadingProgress();
    return true;
}
#endif

uint8_t getCachedBookCount()
{
    return BOOK_COUNT;
}

const char* const* getCachedBookTitles()
{
    return BOOK_TITLES;
}

const CachedBook& getCachedBook(uint8_t index)
{
    return BOOKS[index < BOOK_COUNT ? index : 0];
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
    if (
        !fileSystemReady ||
        strcmp(book.id, TEST_BOOK_ID) != 0
    ) {
        return false;
    }

    currentTextFile.close();
    currentTextFile = LittleFS.open(TEST_BOOK_PATH, "r");
    currentFilePath = nullptr;
    bufferedByteCount = 0;

    if (!currentTextFile)
    {
        Serial.println(F("Missing /lorem.txt"));
        return false;
    }

    if (currentTextFile.size() == 0)
    {
        Serial.println(F("/lorem.txt is empty"));
        currentTextFile.close();
        return false;
    }

    currentFilePath = TEST_BOOK_PATH;
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

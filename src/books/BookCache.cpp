#include "books/BookCache.h"

#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <LittleFS.h>
#endif

namespace
{
#if defined(ARDUINO_ARCH_ESP32)
    constexpr char TEST_BOOK_ID[] = "lorem-ipsum";
    constexpr char TEST_BOOK_PATH[] = "/lorem.txt";
    constexpr size_t TEST_TEXT_BUFFER_SIZE = 2048;

    const CachedBook BOOKS[] = {
        { TEST_BOOK_ID, "Lorem Ipsum" }
    };

    const char* const BOOK_TITLES[] = {
        BOOKS[0].title
    };

    char testTextBuffer[TEST_TEXT_BUFFER_SIZE];
    bool fileSystemReady = false;
#else
    const CachedBook BOOKS[] = {
        { "book-1", "The Hobbit" }
    };

    const char* const BOOK_TITLES[] = {
        BOOKS[0].title
    };

    const char* const BOOK_TEXTS[] = {
        "Then the hobbit slipped on his ring, "
    };
#endif

    constexpr uint8_t BOOK_COUNT =
        sizeof(BOOKS) / sizeof(BOOKS[0]);
}

#if defined(ARDUINO_ARCH_ESP32)
bool initBookCache()
{
    fileSystemReady = LittleFS.begin(false);

    if (!fileSystemReady)
    {
        Serial.println(F("LittleFS unavailable"));
    }

    return fileSystemReady;
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

const char* readCachedBookText(const CachedBook& book)
{
#if defined(ARDUINO_ARCH_ESP32)
    if (
        !fileSystemReady ||
        strcmp(book.id, TEST_BOOK_ID) != 0
    ) {
        return nullptr;
    }

    File textFile = LittleFS.open(TEST_BOOK_PATH, "r");

    if (!textFile)
    {
        Serial.println(F("Missing /lorem.txt"));
        return nullptr;
    }

    const size_t textLength = textFile.readBytes(
        testTextBuffer,
        TEST_TEXT_BUFFER_SIZE - 1
    );

    testTextBuffer[textLength] = '\0';
    textFile.close();

    if (textLength == 0)
    {
        Serial.println(F("/lorem.txt is empty"));
        return nullptr;
    }

    return testTextBuffer;
#else
    for (uint8_t index = 0; index < BOOK_COUNT; index++)
    {
        if (strcmp(book.id, BOOKS[index].id) == 0)
        {
            return BOOK_TEXTS[index];
        }
    }

    return nullptr;
#endif
}

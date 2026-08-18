#include "books/BookCache.h"

#include <stdlib.h>
#include <string.h>

#include <LittleFS.h>

namespace
{
    constexpr char BOOK_MANIFEST_PATH[] = "/books.tsv";
    constexpr char READING_PROGRESS_PATH[] = "/reading-progress.txt";
    constexpr char READING_PROGRESS_TEMP_PATH[] = "/reading-progress.tmp";
    constexpr char READING_PROGRESS_BACKUP_PATH[] = "/reading-progress.bak";
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
    bool progressDirty = false;

    bool fileSystemReady = false;
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

    void loadReadingProgress()
    {
        if (
            !LittleFS.exists(READING_PROGRESS_PATH) &&
            LittleFS.exists(READING_PROGRESS_BACKUP_PATH)
        ) {
            LittleFS.rename(
                READING_PROGRESS_BACKUP_PATH,
                READING_PROGRESS_PATH
            );
        }

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
        progressDirty = false;
    }

    bool writeReadingProgress()
    {
        if (!fileSystemReady)
        {
            return false;
        }

        LittleFS.remove(READING_PROGRESS_TEMP_PATH);
        File progressFile = LittleFS.open(
            READING_PROGRESS_TEMP_PATH,
            "w"
        );

        if (!progressFile)
        {
            Serial.println(F("Could not save reading progress"));
            return false;
        }

        for (uint8_t index = 0; index < bookCount; index++)
        {
            progressFile.print(books[index].id);
            progressFile.print('\t');
            progressFile.println(savedPages[index]);
        }

        progressFile.close();

        LittleFS.remove(READING_PROGRESS_BACKUP_PATH);

        if (
            LittleFS.exists(READING_PROGRESS_PATH) &&
            !LittleFS.rename(
                READING_PROGRESS_PATH,
                READING_PROGRESS_BACKUP_PATH
            )
        ) {
            LittleFS.remove(READING_PROGRESS_TEMP_PATH);
            Serial.println(F("Could not back up reading progress"));
            return false;
        }

        if (!LittleFS.rename(
            READING_PROGRESS_TEMP_PATH,
            READING_PROGRESS_PATH
        )) {
            LittleFS.rename(
                READING_PROGRESS_BACKUP_PATH,
                READING_PROGRESS_PATH
            );
            Serial.println(F("Could not replace reading progress"));
            return false;
        }

        LittleFS.remove(READING_PROGRESS_BACKUP_PATH);
        return true;
    }
}

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

uint8_t getCachedBookCount()
{
    return bookCount;
}

const char* const* getCachedBookTitles()
{
    return bookTitles;
}

const CachedBook& getCachedBook(uint8_t index)
{
    static const CachedBook EMPTY_BOOK = { "", "", "" };
    return index < bookCount ? books[index] : EMPTY_BOOK;
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
    progressDirty = true;
}

bool flushCachedBookProgress()
{
    if (!progressDirty)
    {
        return true;
    }

    if (!writeReadingProgress())
    {
        return false;
    }

    progressDirty = false;
    return true;
}

ReaderDocument::~ReaderDocument()
{
    close();
}

bool ReaderDocument::open(const CachedBook& book)
{
    close();

    if (!fileSystemReady || book.filePath == nullptr || book.filePath[0] == '\0')
    {
        return false;
    }

    file = LittleFS.open(book.filePath, "r");

    if (!file)
    {
        Serial.print(F("Missing book file: "));
        Serial.println(book.filePath);
        return false;
    }

    byteLength = file.size();

    if (byteLength == 0)
    {
        Serial.print(F("Book file is empty: "));
        Serial.println(book.filePath);
        close();
        return false;
    }

    return true;
}

void ReaderDocument::close()
{
    file.close();
    byteLength = 0;
    bufferedByteStart = 0;
    bufferedByteCount = 0;
}

bool ReaderDocument::isOpen() const
{
    return file && byteLength > 0;
}

uint32_t ReaderDocument::length() const
{
    return byteLength;
}

char ReaderDocument::readCharacter(uint32_t position) const
{
    if (!isOpen() || position >= byteLength)
    {
        return '\0';
    }

    const uint32_t bufferedByteEnd =
        bufferedByteStart + bufferedByteCount;

    if (position < bufferedByteStart || position >= bufferedByteEnd)
    {
        if (!file.seek(position))
        {
            return '\0';
        }

        bufferedByteStart = position;
        bufferedByteCount = file.readBytes(
            readBuffer,
            READ_BUFFER_SIZE
        );
    }

    return readBuffer[position - bufferedByteStart];
}

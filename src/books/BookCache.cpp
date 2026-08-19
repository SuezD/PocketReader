#include "books/BookCache.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <LittleFS.h>

namespace
{
    constexpr char BOOK_MANIFEST_PATH[] = "/books.tsv";
    constexpr char BOOK_MANIFEST_TEMP_PATH[] = "/books.tmp";
    constexpr char BOOK_MANIFEST_BACKUP_PATH[] = "/books.bak";
    constexpr char BOOK_DIRECTORY[] = "/books";
    constexpr char READING_PROGRESS_PATH[] = "/reading-progress.txt";
    constexpr char READING_PROGRESS_TEMP_PATH[] = "/reading-progress.tmp";
    constexpr char READING_PROGRESS_BACKUP_PATH[] = "/reading-progress.bak";
    constexpr char CURRENT_BOOK_ENTRY[] = "@current";
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
    int currentBookIndex = -1;
    bool progressDirty = false;

    bool fileSystemReady = false;

    bool littleFsFileExists(const char* path)
    {
        char mountedPath[96];
        const int length = snprintf(
            mountedPath,
            sizeof(mountedPath),
            "/littlefs%s",
            path
        );
        if (length < 0 || static_cast<size_t>(length) >= sizeof(mountedPath))
        {
            return false;
        }

        struct stat fileStatus;
        return stat(mountedPath, &fileStatus) == 0;
    }

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

    int getBookIndexById(const char* id)
    {
        if (id == nullptr) return -1;
        for (uint8_t index = 0; index < bookCount; index++)
        {
            if (strcmp(id, books[index].id) == 0) return index;
        }
        return -1;
    }

    bool isValidBookId(const char* id)
    {
        if (id == nullptr || id[0] == '\0') return false;
        for (size_t index = 0; id[index] != '\0'; index++)
        {
            const char character = id[index];
            if (!(
                (character >= 'a' && character <= 'z') ||
                (character >= 'A' && character <= 'Z') ||
                (character >= '0' && character <= '9') ||
                character == '-' || character == '_'
            )) {
                return false;
            }
        }
        return true;
    }

    bool writeBookManifest()
    {
        if (littleFsFileExists(BOOK_MANIFEST_TEMP_PATH))
        {
            LittleFS.remove(BOOK_MANIFEST_TEMP_PATH);
        }
        File manifest = LittleFS.open(BOOK_MANIFEST_TEMP_PATH, "w");
        if (!manifest) return false;

        for (uint8_t index = 0; index < bookCount; index++)
        {
            manifest.print(books[index].id);
            manifest.print('\t');
            manifest.print(books[index].title);
            manifest.print('\t');
            manifest.println(books[index].filePath);
        }
        manifest.close();

        if (littleFsFileExists(BOOK_MANIFEST_BACKUP_PATH))
        {
            LittleFS.remove(BOOK_MANIFEST_BACKUP_PATH);
        }
        const bool hadManifest = littleFsFileExists(BOOK_MANIFEST_PATH);
        if (
            hadManifest &&
            !LittleFS.rename(BOOK_MANIFEST_PATH, BOOK_MANIFEST_BACKUP_PATH)
        ) {
            LittleFS.remove(BOOK_MANIFEST_TEMP_PATH);
            return false;
        }
        if (!LittleFS.rename(BOOK_MANIFEST_TEMP_PATH, BOOK_MANIFEST_PATH))
        {
            if (hadManifest)
            {
                LittleFS.rename(
                    BOOK_MANIFEST_BACKUP_PATH,
                    BOOK_MANIFEST_PATH
                );
            }
            if (littleFsFileExists(BOOK_MANIFEST_TEMP_PATH))
            {
                LittleFS.remove(BOOK_MANIFEST_TEMP_PATH);
            }
            return false;
        }
        if (littleFsFileExists(BOOK_MANIFEST_BACKUP_PATH))
        {
            LittleFS.remove(BOOK_MANIFEST_BACKUP_PATH);
        }
        return true;
    }

    void removeLastBookRecord()
    {
        if (bookCount == 0) return;
        bookCount--;
        books[bookCount] = { nullptr, nullptr, nullptr };
        bookTitles[bookCount] = nullptr;
        bookIds[bookCount][0] = '\0';
        bookTitleStorage[bookCount][0] = '\0';
        bookPaths[bookCount][0] = '\0';
        savedPages[bookCount] = 0;
    }

    void removeBookRecord(uint8_t removedIndex)
    {
        if (removedIndex >= bookCount) return;

        for (uint8_t index = removedIndex; index + 1 < bookCount; index++)
        {
            memcpy(bookIds[index], bookIds[index + 1], sizeof(bookIds[index]));
            memcpy(
                bookTitleStorage[index],
                bookTitleStorage[index + 1],
                sizeof(bookTitleStorage[index])
            );
            memcpy(
                bookPaths[index],
                bookPaths[index + 1],
                sizeof(bookPaths[index])
            );
            savedPages[index] = savedPages[index + 1];
        }

        if (currentBookIndex == removedIndex)
        {
            currentBookIndex = -1;
        }
        else if (currentBookIndex > removedIndex)
        {
            currentBookIndex--;
        }

        removeLastBookRecord();
        for (uint8_t index = 0; index < bookCount; index++)
        {
            books[index] = {
                bookIds[index],
                bookTitleStorage[index],
                bookPaths[index]
            };
            bookTitles[index] = books[index].title;
        }
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

        if (
            !littleFsFileExists(BOOK_MANIFEST_PATH) &&
            littleFsFileExists(BOOK_MANIFEST_BACKUP_PATH)
        ) {
            LittleFS.rename(
                BOOK_MANIFEST_BACKUP_PATH,
                BOOK_MANIFEST_PATH
            );
        }
        if (littleFsFileExists(BOOK_MANIFEST_TEMP_PATH))
        {
            LittleFS.remove(BOOK_MANIFEST_TEMP_PATH);
        }
        if (
            littleFsFileExists(BOOK_MANIFEST_PATH) &&
            littleFsFileExists(BOOK_MANIFEST_BACKUP_PATH)
        ) {
            LittleFS.remove(BOOK_MANIFEST_BACKUP_PATH);
        }

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
        currentBookIndex = -1;

        if (
            !littleFsFileExists(READING_PROGRESS_PATH) &&
            littleFsFileExists(READING_PROGRESS_BACKUP_PATH)
        ) {
            LittleFS.rename(
                READING_PROGRESS_BACKUP_PATH,
                READING_PROGRESS_PATH
            );
        }

        if (!littleFsFileExists(READING_PROGRESS_PATH))
        {
            progressDirty = false;
            return;
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

            if (entryLength > 0 && entry[entryLength - 1] == '\r')
            {
                entry[entryLength - 1] = '\0';
            }

            char* separator = strchr(entry, '\t');

            if (separator == nullptr)
            {
                continue;
            }

            *separator = '\0';

            if (strcmp(entry, CURRENT_BOOK_ENTRY) == 0)
            {
                for (uint8_t index = 0; index < bookCount; index++)
                {
                    if (strcmp(separator + 1, books[index].id) == 0)
                    {
                        currentBookIndex = index;
                        break;
                    }
                }

                continue;
            }

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

        if (littleFsFileExists(READING_PROGRESS_TEMP_PATH))
        {
            LittleFS.remove(READING_PROGRESS_TEMP_PATH);
        }
        File progressFile = LittleFS.open(
            READING_PROGRESS_TEMP_PATH,
            "w"
        );

        if (!progressFile)
        {
            Serial.println(F("Could not save reading progress"));
            return false;
        }

        if (currentBookIndex >= 0 && currentBookIndex < bookCount)
        {
            progressFile.print(CURRENT_BOOK_ENTRY);
            progressFile.print('\t');
            progressFile.println(books[currentBookIndex].id);
        }

        for (uint8_t index = 0; index < bookCount; index++)
        {
            progressFile.print(books[index].id);
            progressFile.print('\t');
            progressFile.println(savedPages[index]);
        }

        progressFile.close();

        if (littleFsFileExists(READING_PROGRESS_BACKUP_PATH))
        {
            LittleFS.remove(READING_PROGRESS_BACKUP_PATH);
        }

        if (
            littleFsFileExists(READING_PROGRESS_PATH) &&
            !LittleFS.rename(
                READING_PROGRESS_PATH,
                READING_PROGRESS_BACKUP_PATH
            )
        ) {
            if (littleFsFileExists(READING_PROGRESS_TEMP_PATH))
            {
                LittleFS.remove(READING_PROGRESS_TEMP_PATH);
            }
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

        if (littleFsFileExists(READING_PROGRESS_BACKUP_PATH))
        {
            LittleFS.remove(READING_PROGRESS_BACKUP_PATH);
        }
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

BookInstallResult installCachedBook(
    const char* id,
    const char* title,
    const char* temporaryPath
)
{
    if (
        !fileSystemReady || !isValidBookId(id) ||
        title == nullptr || title[0] == '\0' ||
        strlen(id) > MAX_BOOK_ID_LENGTH ||
        strlen(title) > MAX_BOOK_TITLE_LENGTH
    ) {
        return BookInstallResult::InvalidBook;
    }
    if (getBookIndexById(id) >= 0)
    {
        if (temporaryPath != nullptr) LittleFS.remove(temporaryPath);
        return BookInstallResult::AlreadyCached;
    }
    if (bookCount >= MAX_BOOK_COUNT) return BookInstallResult::LibraryFull;
    if (
        temporaryPath == nullptr ||
        !littleFsFileExists(temporaryPath)
    ) {
        return BookInstallResult::MissingDownload;
    }

    File downloadedFile = LittleFS.open(temporaryPath, "r");
    if (!downloadedFile || downloadedFile.size() == 0)
    {
        downloadedFile.close();
        LittleFS.remove(temporaryPath);
        return BookInstallResult::MissingDownload;
    }
    downloadedFile.close();

    if (!littleFsFileExists(BOOK_DIRECTORY) && !LittleFS.mkdir(BOOK_DIRECTORY))
    {
        return BookInstallResult::StorageError;
    }

    char finalPath[MAX_BOOK_PATH_LENGTH + 1];
    const int pathLength = snprintf(
        finalPath,
        sizeof(finalPath),
        "%s/%s.txt",
        BOOK_DIRECTORY,
        id
    );
    if (pathLength < 0 || pathLength >= static_cast<int>(sizeof(finalPath)))
    {
        return BookInstallResult::InvalidBook;
    }
    if (littleFsFileExists(finalPath)) LittleFS.remove(finalPath);
    if (!LittleFS.rename(temporaryPath, finalPath))
    {
        return BookInstallResult::StorageError;
    }

    char manifestLine[MAX_MANIFEST_LINE_LENGTH + 1];
    const int lineLength = snprintf(
        manifestLine,
        sizeof(manifestLine),
        "%s\t%s\t%s",
        id,
        title,
        finalPath
    );
    if (
        lineLength < 0 ||
        lineLength >= static_cast<int>(sizeof(manifestLine)) ||
        !addManifestBook(manifestLine)
    ) {
        LittleFS.remove(finalPath);
        return BookInstallResult::InvalidBook;
    }

    if (!writeBookManifest())
    {
        removeLastBookRecord();
        LittleFS.remove(finalPath);
        return BookInstallResult::ManifestError;
    }

    Serial.print(F("Installed cached book: "));
    Serial.print(title);
    Serial.print(F(" -> "));
    Serial.println(finalPath);
    return BookInstallResult::Success;
}

const char* getBookInstallResultText(BookInstallResult result)
{
    switch (result)
    {
        case BookInstallResult::Success: return "Added to My Books";
        case BookInstallResult::AlreadyCached: return "Already in My Books";
        case BookInstallResult::LibraryFull: return "My Books is full";
        case BookInstallResult::InvalidBook: return "Invalid book details";
        case BookInstallResult::MissingDownload: return "Downloaded file is missing";
        case BookInstallResult::StorageError: return "Could not store book";
        case BookInstallResult::ManifestError: return "Could not update library";
    }
    return "Could not add book";
}

BookDeleteResult deleteCachedBook(const char* id)
{
    if (!fileSystemReady) return BookDeleteResult::StorageError;

    const int bookIndex = getBookIndexById(id);
    if (bookIndex < 0) return BookDeleteResult::NotFound;

    char removedPath[MAX_BOOK_PATH_LENGTH + 1];
    memcpy(
        removedPath,
        bookPaths[bookIndex],
        sizeof(removedPath)
    );

    removeBookRecord(static_cast<uint8_t>(bookIndex));
    if (!writeBookManifest())
    {
        loadBookManifest();
        loadReadingProgress();
        return BookDeleteResult::ManifestError;
    }

    progressDirty = true;
    const bool progressSaved = flushCachedBookProgress();
    const bool fileRemoved =
        !littleFsFileExists(removedPath) || LittleFS.remove(removedPath);

    Serial.print(F("Deleted cached book: "));
    Serial.println(id);

    if (!fileRemoved) return BookDeleteResult::StorageError;
    if (!progressSaved) return BookDeleteResult::ProgressError;
    return BookDeleteResult::Success;
}

const char* getBookDeleteResultText(BookDeleteResult result)
{
    switch (result)
    {
        case BookDeleteResult::Success: return "Book deleted";
        case BookDeleteResult::NotFound: return "Book already removed";
        case BookDeleteResult::StorageError: return "Book removed; file remains";
        case BookDeleteResult::ManifestError: return "Could not update library";
        case BookDeleteResult::ProgressError: return "Book removed; progress remains";
    }
    return "Could not delete book";
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

const CachedBook* findCachedBook(const char* id)
{
    const int index = getBookIndexById(id);
    return index >= 0 ? &books[index] : nullptr;
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

const CachedBook* getLastOpenedCachedBook()
{
    return
        currentBookIndex >= 0 && currentBookIndex < bookCount
            ? &books[currentBookIndex]
            : nullptr;
}

void setLastOpenedCachedBook(const CachedBook& book)
{
    const int index = getBookIndex(book);

    if (index < 0 || index == currentBookIndex)
    {
        return;
    }

    currentBookIndex = index;
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

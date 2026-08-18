#include "books/BookSync.h"

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <string.h>

#include "books/BookServerConfig.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr uint16_t REQUEST_TIMEOUT_MS = 8000;

    bool copyField(
        char* destination,
        size_t destinationSize,
        const char* source
    ) {
        const size_t length = strlen(source);
        if (length == 0 || length >= destinationSize) return false;
        memcpy(destination, source, length + 1);
        return true;
    }

    bool isDownloadUrl(const char* value)
    {
        return strncmp(value, "http://", 7) == 0 ||
            strncmp(value, "https://", 8) == 0;
    }
}

BookSyncResult BookSync::fetchManifest()
{
    clear();
    if (DevelopmentBookServer::MANIFEST_URL[0] == '\0')
    {
        return BookSyncResult::NotConfigured;
    }
    if (!getWifiManager().isConnected())
    {
        return BookSyncResult::NotConnected;
    }

    const String manifestUrl = DevelopmentBookServer::MANIFEST_URL;
    const bool usesTls = manifestUrl.startsWith("https://");
    if (usesTls && DevelopmentBookServer::ROOT_CA[0] == '\0')
    {
        return BookSyncResult::TlsConfigurationMissing;
    }
    if (!usesTls && !manifestUrl.startsWith("http://"))
    {
        return BookSyncResult::NotConfigured;
    }

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    WiFiClient* client = &plainClient;
    if (usesTls)
    {
        secureClient.setCACert(DevelopmentBookServer::ROOT_CA);
        client = &secureClient;
    }

    HTTPClient request;
    request.setConnectTimeout(REQUEST_TIMEOUT_MS);
    request.setTimeout(REQUEST_TIMEOUT_MS);
    request.setUserAgent("PocketReader/1.0");
    if (!request.begin(*client, manifestUrl))
    {
        return BookSyncResult::RequestFailed;
    }

    httpStatus = request.GET();
    if (httpStatus <= 0)
    {
        request.end();
        return BookSyncResult::RequestFailed;
    }
    if (httpStatus < 200 || httpStatus >= 300)
    {
        request.end();
        return BookSyncResult::HttpError;
    }

    const int responseSize = request.getSize();
    if (
        responseSize > 0 &&
        static_cast<size_t>(responseSize) > MAX_MANIFEST_BYTES
    ) {
        request.end();
        return BookSyncResult::ManifestTooLarge;
    }

    const String manifest = request.getString();
    request.end();
    if (manifest.length() > MAX_MANIFEST_BYTES)
    {
        return BookSyncResult::ManifestTooLarge;
    }
    if (!parseManifest(manifest))
    {
        clear();
        return BookSyncResult::InvalidManifest;
    }
    return BookSyncResult::Success;
}

uint8_t BookSync::getBookCount() const
{
    return bookCount;
}

const RemoteBook& BookSync::getBook(uint8_t index) const
{
    static const RemoteBook EMPTY_BOOK = { "", "", "" };
    return index < bookCount ? books[index] : EMPTY_BOOK;
}

int BookSync::getHttpStatus() const
{
    return httpStatus;
}

void BookSync::clear()
{
    bookCount = 0;
    httpStatus = 0;
}

bool BookSync::parseManifest(const String& manifest)
{
    size_t lineStart = 0;
    while (lineStart < manifest.length())
    {
        size_t lineEnd = manifest.indexOf('\n', lineStart);
        if (lineEnd == static_cast<size_t>(-1)) lineEnd = manifest.length();
        size_t lineLength = lineEnd - lineStart;
        if (lineLength > 0 && manifest[lineEnd - 1] == '\r') lineLength--;

        if (lineLength > 0)
        {
            if (lineLength > MAX_LINE_LENGTH || bookCount >= MAX_BOOK_COUNT)
            {
                return false;
            }
            char line[MAX_LINE_LENGTH + 1];
            manifest.substring(lineStart, lineStart + lineLength)
                .toCharArray(line, sizeof(line));
            if (!addBook(line)) return false;
        }
        lineStart = lineEnd + 1;
    }
    return true;
}

bool BookSync::addBook(char* line)
{
    char* firstTab = strchr(line, '\t');
    if (firstTab == nullptr) return false;
    *firstTab = '\0';
    char* title = firstTab + 1;
    char* secondTab = strchr(title, '\t');
    if (secondTab == nullptr) return false;
    *secondTab = '\0';
    char* url = secondTab + 1;
    if (strchr(url, '\t') != nullptr || !isDownloadUrl(url)) return false;

    for (uint8_t index = 0; index < bookCount; index++)
    {
        if (strcmp(ids[index], line) == 0) return false;
    }
    if (
        !copyField(ids[bookCount], sizeof(ids[bookCount]), line) ||
        !copyField(titles[bookCount], sizeof(titles[bookCount]), title) ||
        !copyField(urls[bookCount], sizeof(urls[bookCount]), url)
    ) {
        return false;
    }
    books[bookCount] = {
        ids[bookCount], titles[bookCount], urls[bookCount]
    };
    bookCount++;
    return true;
}

BookSync& getBookSync()
{
    static BookSync sync;
    return sync;
}

const char* getBookSyncResultText(BookSyncResult result)
{
    switch (result)
    {
        case BookSyncResult::Success: return "Server manifest loaded";
        case BookSyncResult::NotConfigured: return "Book server not configured";
        case BookSyncResult::NotConnected: return "Connect to Wi-Fi first";
        case BookSyncResult::TlsConfigurationMissing: return "Server certificate missing";
        case BookSyncResult::RequestFailed: return "Could not reach book server";
        case BookSyncResult::HttpError: return "Book server returned an error";
        case BookSyncResult::ManifestTooLarge: return "Server manifest is too large";
        case BookSyncResult::InvalidManifest: return "Server manifest is invalid";
    }
    return "Book sync failed";
}

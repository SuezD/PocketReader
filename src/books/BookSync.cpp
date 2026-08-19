#include "books/BookSync.h"

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <stdlib.h>
#include <string.h>

#include <LittleFS.h>

#include "books/BookServerConfig.h"
#include "books/BookServerSettings.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr uint16_t REQUEST_TIMEOUT_MS = 8000;
    constexpr char TEMPORARY_DOWNLOAD_PATH[] = "/book-download.tmp";

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

    void addAuthorizationHeader(HTTPClient& request)
    {
        const char* accessToken = getBookServerSettings().getAccessToken();
        if (accessToken[0] == '\0') return;

        String authorization = "Bearer ";
        authorization += accessToken;
        request.addHeader("Authorization", authorization);
    }
}

BookSyncResult BookSync::fetchManifest()
{
    clear();
    const char* configuredUrl = getBookServerSettings().getManifestUrl();
    if (configuredUrl[0] == '\0')
    {
        return BookSyncResult::NotConfigured;
    }
    if (!getWifiManager().isConnected())
    {
        return BookSyncResult::NotConnected;
    }

    const String manifestUrl = configuredUrl;
    const bool usesTls = manifestUrl.startsWith("https://");
    if (usesTls && BookServerConfig::ROOT_CA[0] == '\0')
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
        secureClient.setCACert(BookServerConfig::ROOT_CA);
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
    addAuthorizationHeader(request);

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

BookDownloadResult BookSync::downloadBook(const RemoteBook& book)
{
    httpStatus = 0;
    if (!getWifiManager().isConnected())
    {
        return BookDownloadResult::NotConnected;
    }

    const String downloadUrl = book.downloadUrl == nullptr
        ? ""
        : book.downloadUrl;
    const bool usesTls = downloadUrl.startsWith("https://");
    if (usesTls && BookServerConfig::ROOT_CA[0] == '\0')
    {
        return BookDownloadResult::TlsConfigurationMissing;
    }
    if (!usesTls && !downloadUrl.startsWith("http://"))
    {
        return BookDownloadResult::RequestFailed;
    }

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    WiFiClient* client = &plainClient;
    if (usesTls)
    {
        secureClient.setCACert(BookServerConfig::ROOT_CA);
        client = &secureClient;
    }

    HTTPClient request;
    request.setConnectTimeout(REQUEST_TIMEOUT_MS);
    request.setTimeout(REQUEST_TIMEOUT_MS);
    request.setUserAgent("PocketReader/1.0");
    if (!request.begin(*client, downloadUrl))
    {
        return BookDownloadResult::RequestFailed;
    }
    addAuthorizationHeader(request);

    httpStatus = request.GET();
    if (httpStatus <= 0)
    {
        request.end();
        return BookDownloadResult::RequestFailed;
    }
    if (httpStatus < 200 || httpStatus >= 300)
    {
        request.end();
        return BookDownloadResult::HttpError;
    }

    const int expectedSize = request.getSize();
    const size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
    if (
        expectedSize > 0 &&
        static_cast<size_t>(expectedSize) > freeBytes
    ) {
        request.end();
        return BookDownloadResult::NotEnoughSpace;
    }

    File output = LittleFS.open(TEMPORARY_DOWNLOAD_PATH, "w");
    if (!output)
    {
        request.end();
        return BookDownloadResult::StorageError;
    }

    const int writtenBytes = request.writeToStream(&output);
    output.close();
    request.end();

    if (writtenBytes <= 0)
    {
        LittleFS.remove(TEMPORARY_DOWNLOAD_PATH);
        return writtenBytes == 0
            ? BookDownloadResult::EmptyFile
            : BookDownloadResult::RequestFailed;
    }
    if (expectedSize >= 0 && writtenBytes != expectedSize)
    {
        LittleFS.remove(TEMPORARY_DOWNLOAD_PATH);
        return BookDownloadResult::IncompleteDownload;
    }
    return BookDownloadResult::Success;
}

uint8_t BookSync::getBookCount() const
{
    return bookCount;
}

const RemoteBook& BookSync::getBook(uint8_t index) const
{
    static const RemoteBook EMPTY_BOOK = { "", "", "", 0 };
    return index < bookCount ? books[index] : EMPTY_BOOK;
}

int BookSync::getHttpStatus() const
{
    return httpStatus;
}

const char* BookSync::getTemporaryDownloadPath() const
{
    return TEMPORARY_DOWNLOAD_PATH;
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
    char* thirdTab = strchr(url, '\t');
    uint32_t sizeBytes = 0;
    if (thirdTab != nullptr)
    {
        *thirdTab = '\0';
        const char* sizeText = thirdTab + 1;
        char* sizeEnd = nullptr;
        const unsigned long parsedSize = strtoul(sizeText, &sizeEnd, 10);
        if (
            sizeText[0] == '\0' || sizeEnd == nullptr ||
            sizeEnd[0] != '\0' || parsedSize == 0 ||
            parsedSize > UINT32_MAX
        ) {
            return false;
        }
        sizeBytes = static_cast<uint32_t>(parsedSize);
    }
    if (!isDownloadUrl(url)) return false;

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
        ids[bookCount], titles[bookCount], urls[bookCount], sizeBytes
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

const char* getBookDownloadResultText(BookDownloadResult result)
{
    switch (result)
    {
        case BookDownloadResult::Success: return "Download complete";
        case BookDownloadResult::NotConnected: return "Wi-Fi disconnected";
        case BookDownloadResult::TlsConfigurationMissing:
            return "Server certificate missing";
        case BookDownloadResult::RequestFailed: return "Download failed";
        case BookDownloadResult::HttpError: return "Book server returned an error";
        case BookDownloadResult::NotEnoughSpace: return "Not enough storage";
        case BookDownloadResult::StorageError: return "Could not save download";
        case BookDownloadResult::EmptyFile: return "Downloaded book was empty";
        case BookDownloadResult::IncompleteDownload:
            return "Download was interrupted";
    }
    return "Download failed";
}

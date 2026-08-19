#include "books/BookSync.h"

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <LittleFS.h>

#include "books/BookServerSettings.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr uint16_t REQUEST_TIMEOUT_MS = 8000;
    constexpr uint16_t CLOCK_SYNC_TIMEOUT_MS = 8000;
    constexpr time_t MINIMUM_VALID_TIME = 1704067200;
    constexpr char TEMPORARY_DOWNLOAD_PATH[] = "/book-download.tmp";

    extern const uint8_t ESP_IDF_CA_BUNDLE[]
        asm("_binary_x509_crt_bundle_start");

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

    bool ensureClockSynchronized()
    {
        if (time(nullptr) >= MINIMUM_VALID_TIME) return true;

        Serial.println(F("[BookSync] Synchronising clock for HTTPS"));
        configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
        const unsigned long startedAt = millis();
        while (
            time(nullptr) < MINIMUM_VALID_TIME &&
            millis() - startedAt < CLOCK_SYNC_TIMEOUT_MS
        ) {
            delay(100);
        }

        if (time(nullptr) >= MINIMUM_VALID_TIME) return true;
        Serial.println(F("[BookSync] Clock synchronisation timed out"));
        return false;
    }

    void configureTrustedTls(WiFiClientSecure& client)
    {
        client.setCACertBundle(ESP_IDF_CA_BUNDLE);
    }

    void logSecureConnectionFailure(WiFiClientSecure& client)
    {
        char error[128] = {};
        const int errorCode = client.lastError(error, sizeof(error));
        Serial.print(F("[BookSync] HTTPS connection failed: "));
        Serial.print(errorCode);
        if (error[0] != '\0')
        {
            Serial.print(F(" ("));
            Serial.print(error);
            Serial.print(')');
        }
        Serial.println();
    }
}

BookSyncResult BookSync::fetchManifest()
{
    clear();
    manifestServerRevision = getBookServerSettings().getRevision();
    const char* configuredUrl = getBookServerSettings().getManifestUrl();
    if (configuredUrl[0] == '\0')
    {
        return finishManifestFetch(BookSyncResult::NotConfigured);
    }
    if (!getWifiManager().isConnected())
    {
        return finishManifestFetch(BookSyncResult::NotConnected);
    }

    const String manifestUrl = configuredUrl;
    const bool usesTls = manifestUrl.startsWith("https://");
    if (!usesTls && !manifestUrl.startsWith("http://"))
    {
        return finishManifestFetch(BookSyncResult::NotConfigured);
    }

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    WiFiClient* client = &plainClient;
    if (usesTls)
    {
        if (!ensureClockSynchronized())
        {
            return finishManifestFetch(
                BookSyncResult::ClockNotSynchronized
            );
        }
        configureTrustedTls(secureClient);
        client = &secureClient;
    }

    HTTPClient request;
    request.setConnectTimeout(REQUEST_TIMEOUT_MS);
    request.setTimeout(REQUEST_TIMEOUT_MS);
    request.setUserAgent("PocketReader/1.0");
    if (!request.begin(*client, manifestUrl))
    {
        return finishManifestFetch(BookSyncResult::RequestFailed);
    }
    addAuthorizationHeader(request);

    httpStatus = request.GET();
    if (httpStatus <= 0)
    {
        if (usesTls) logSecureConnectionFailure(secureClient);
        request.end();
        return finishManifestFetch(
            usesTls
                ? BookSyncResult::SecureConnectionFailed
                : BookSyncResult::RequestFailed
        );
    }
    if (httpStatus < 200 || httpStatus >= 300)
    {
        request.end();
        return finishManifestFetch(BookSyncResult::HttpError);
    }

    const int responseSize = request.getSize();
    if (
        responseSize > 0 &&
        static_cast<size_t>(responseSize) > MAX_MANIFEST_BYTES
    ) {
        request.end();
        return finishManifestFetch(BookSyncResult::ManifestTooLarge);
    }

    const String manifest = request.getString();
    request.end();
    if (manifest.length() > MAX_MANIFEST_BYTES)
    {
        return finishManifestFetch(BookSyncResult::ManifestTooLarge);
    }
    if (!parseManifest(manifest))
    {
        clear();
        return finishManifestFetch(BookSyncResult::InvalidManifest);
    }
    return finishManifestFetch(BookSyncResult::Success);
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
    if (!usesTls && !downloadUrl.startsWith("http://"))
    {
        return BookDownloadResult::RequestFailed;
    }

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    WiFiClient* client = &plainClient;
    if (usesTls)
    {
        if (!ensureClockSynchronized())
        {
            return BookDownloadResult::ClockNotSynchronized;
        }
        configureTrustedTls(secureClient);
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
        if (usesTls) logSecureConnectionFailure(secureClient);
        request.end();
        return usesTls
            ? BookDownloadResult::SecureConnectionFailed
            : BookDownloadResult::RequestFailed;
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

bool BookSync::hasManifestResultFor(uint32_t serverRevision) const
{
    return
        manifestFetchAttempted &&
        manifestServerRevision == serverRevision;
}

BookSyncResult BookSync::getLastManifestResult() const
{
    return lastManifestResult;
}

BookSyncResult BookSync::finishManifestFetch(BookSyncResult result)
{
    manifestFetchAttempted = true;
    lastManifestResult = result;
    return result;
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
        case BookSyncResult::ClockNotSynchronized: return "Could not verify server time";
        case BookSyncResult::SecureConnectionFailed: return "Secure connection failed";
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
        case BookDownloadResult::ClockNotSynchronized:
            return "Could not verify server time";
        case BookDownloadResult::SecureConnectionFailed:
            return "Secure connection failed";
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

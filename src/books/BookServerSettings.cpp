#include "books/BookServerSettings.h"

#include <string.h>

namespace
{
    constexpr char PREFERENCES_NAMESPACE[] = "book-server";
    constexpr char MANIFEST_URL_KEY[] = "manifest-url";
    constexpr char ACCESS_TOKEN_KEY[] = "access-token";
}

void BookServerSettings::begin()
{
    if (ready) return;

    ready = preferences.begin(PREFERENCES_NAMESPACE, false);
    if (!ready)
    {
        Serial.println(F("[BookServer] Could not open preferences"));
        return;
    }

    const String savedUrl = preferences.getString(MANIFEST_URL_KEY, "");
    if (isValidUrl(savedUrl.c_str()))
    {
        snprintf(manifestUrl, sizeof(manifestUrl), "%s", savedUrl.c_str());
        Serial.print(F("[BookServer] Loaded manifest URL: "));
        Serial.println(manifestUrl);
    }
    else if (savedUrl.length() > 0)
    {
        Serial.println(F("[BookServer] Ignored invalid saved manifest URL"));
    }

    const String savedAccessToken = preferences.getString(ACCESS_TOKEN_KEY, "");
    if (isValidAccessToken(savedAccessToken.c_str()))
    {
        snprintf(
            accessToken,
            sizeof(accessToken),
            "%s",
            savedAccessToken.c_str()
        );
        if (accessToken[0] != '\0')
        {
            Serial.println(F("[BookServer] Loaded saved access token"));
        }
    }
    else
    {
        Serial.println(F("[BookServer] Ignored invalid saved access token"));
    }
}

const char* BookServerSettings::getManifestUrl() const
{
    return manifestUrl;
}

const char* BookServerSettings::getAccessToken() const
{
    return accessToken;
}

bool BookServerSettings::hasAccessToken() const
{
    return accessToken[0] != '\0';
}

bool BookServerSettings::save(
    const char* url,
    const char* nextAccessToken
) {
    if (
        !ready || !isValidUrl(url) ||
        !isValidAccessToken(nextAccessToken)
    ) {
        return false;
    }

    const bool urlChanged = strcmp(manifestUrl, url) != 0;
    const bool tokenChanged = strcmp(accessToken, nextAccessToken) != 0;
    if (!urlChanged && !tokenChanged) return true;

    if (tokenChanged)
    {
        if (nextAccessToken[0] == '\0')
        {
            if (!preferences.remove(ACCESS_TOKEN_KEY)) return false;
        }
        else if (
            preferences.putString(ACCESS_TOKEN_KEY, nextAccessToken) == 0
        ) {
            return false;
        }
    }

    if (
        urlChanged && preferences.putString(MANIFEST_URL_KEY, url) == 0
    ) {
        return false;
    }

    if (urlChanged) snprintf(manifestUrl, sizeof(manifestUrl), "%s", url);
    if (tokenChanged)
    {
        snprintf(accessToken, sizeof(accessToken), "%s", nextAccessToken);
    }
    revision++;
    Serial.println(F("[BookServer] Saved server configuration"));
    return true;
}

uint32_t BookServerSettings::getRevision() const
{
    return revision;
}

bool BookServerSettings::isValidUrl(const char* url) const
{
    if (url == nullptr) return false;
    const size_t length = strlen(url);
    if (length == 0 || length > MAX_URL_LENGTH) return false;
    if (
        strncmp(url, "http://", 7) != 0 &&
        strncmp(url, "https://", 8) != 0
    ) {
        return false;
    }

    for (size_t index = 0; index < length; index++)
    {
        if (static_cast<uint8_t>(url[index]) <= 0x20) return false;
    }
    return true;
}

bool BookServerSettings::isValidAccessToken(const char* token) const
{
    if (token == nullptr) return false;
    const size_t length = strlen(token);
    if (length > MAX_ACCESS_TOKEN_LENGTH) return false;
    for (size_t index = 0; index < length; index++)
    {
        if (static_cast<uint8_t>(token[index]) <= 0x20) return false;
    }
    return true;
}

BookServerSettings& getBookServerSettings()
{
    static BookServerSettings settings;
    return settings;
}

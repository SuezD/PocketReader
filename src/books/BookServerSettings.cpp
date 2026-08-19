#include "books/BookServerSettings.h"

#include <string.h>

namespace
{
    constexpr char PREFERENCES_NAMESPACE[] = "book-server";
    constexpr char MANIFEST_URL_KEY[] = "manifest-url";
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
}

const char* BookServerSettings::getManifestUrl() const
{
    return manifestUrl;
}

bool BookServerSettings::setManifestUrl(const char* url)
{
    if (!ready || !isValidUrl(url)) return false;
    if (strcmp(manifestUrl, url) == 0) return true;

    if (preferences.putString(MANIFEST_URL_KEY, url) == 0) return false;
    snprintf(manifestUrl, sizeof(manifestUrl), "%s", url);
    revision++;
    Serial.print(F("[BookServer] Saved manifest URL: "));
    Serial.println(manifestUrl);
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

BookServerSettings& getBookServerSettings()
{
    static BookServerSettings settings;
    return settings;
}

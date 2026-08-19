#pragma once

#include <Arduino.h>
#include <Preferences.h>

class BookServerSettings
{
public:
    static constexpr size_t MAX_URL_LENGTH = 255;
    static constexpr size_t MAX_ACCESS_TOKEN_LENGTH = 255;

    void begin();
    const char* getManifestUrl() const;
    const char* getAccessToken() const;
    bool hasAccessToken() const;
    bool save(
        const char* url,
        const char* accessToken
    );
    uint32_t getRevision() const;

private:
    Preferences preferences;
    bool ready = false;
    char manifestUrl[MAX_URL_LENGTH + 1] = {};
    char accessToken[MAX_ACCESS_TOKEN_LENGTH + 1] = {};
    uint32_t revision = 0;

    bool isValidUrl(const char* url) const;
    bool isValidAccessToken(const char* token) const;
};

BookServerSettings& getBookServerSettings();

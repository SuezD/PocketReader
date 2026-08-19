#pragma once

#include <Arduino.h>
#include <Preferences.h>

class BookServerSettings
{
public:
    static constexpr size_t MAX_URL_LENGTH = 255;

    void begin();
    const char* getManifestUrl() const;
    bool setManifestUrl(const char* url);
    uint32_t getRevision() const;

private:
    Preferences preferences;
    bool ready = false;
    char manifestUrl[MAX_URL_LENGTH + 1] = {};
    uint32_t revision = 0;

    bool isValidUrl(const char* url) const;
};

BookServerSettings& getBookServerSettings();

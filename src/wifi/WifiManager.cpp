#include "wifi/WifiManager.h"

#include <WiFi.h>

namespace
{
    constexpr char SETUP_NETWORK_NAME[] = "PocketReader-Setup";
    constexpr char SETUP_NETWORK_PASSWORD[] = "pocketreader";

    const __FlashStringHelper* getStateName(WifiState state)
    {
        switch (state)
        {
            case WifiState::Disconnected:
                return F("disconnected");
            case WifiState::Connecting:
                return F("connecting");
            case WifiState::Connected:
                return F("connected");
            case WifiState::Failed:
                return F("failed");
        }

        return F("unknown");
    }
}

void WifiManager::begin()
{
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    setState(WifiState::Disconnected);

    preferencesReady = preferences.begin("pocket-wifi", false);

    if (!preferencesReady)
    {
        Serial.println(F("Could not open Wi-Fi preferences"));
        return;
    }

    loadSavedNetworks();
    savedNetworkAvailable = savedNetworkCount > 0;
    Serial.print(F("Saved Wi-Fi networks: "));
    Serial.println(savedNetworkCount);

    if (savedNetworkAvailable)
    {
        networkName = savedNetworkNames[0];
        networkPassword = savedNetworkPasswords[0];
        Serial.print(F("Loaded saved Wi-Fi network: "));
        Serial.println(networkName);
        startupSelectionActive = true;
        saveAfterConnection = false;
        startSavedNetworkScan();
    }
}

void WifiManager::connect(const char* ssid, const char* password)
{
    if (ssid == nullptr || ssid[0] == '\0')
    {
        Serial.println(F("Wi-Fi credentials are not configured"));
        return;
    }

    networkName = ssid;
    networkPassword = password == nullptr ? "" : password;
    if (savedNetworkScanPending)
    {
        savedNetworkScanPending = false;
        retryAt = 0;
        setState(WifiState::Disconnected);
    }
    visibleSavedNetworkCount = 0;
    nextVisibleSavedNetwork = 0;
    startupSelectionActive = false;
    saveAfterConnection = true;
    WiFi.disconnect(false, false);
    retryAt = millis() + CONNECTION_RESET_MS;
    connectionResetPending = true;
    setState(WifiState::Disconnected);
}

void WifiManager::disconnect()
{
    startupSelectionActive = false;
    if (savedNetworkScanActive)
    {
        WiFi.scanDelete();
        savedNetworkScanActive = false;
    }
    savedNetworkScanPending = false;
    connectionResetPending = false;
    retryAt = 0;
    WiFi.disconnect(false, false);
    setState(WifiState::Disconnected);
}

bool WifiManager::connectSavedNetwork(const char* ssid)
{
    if (ssid == nullptr)
    {
        return false;
    }

    const int index = findSavedNetwork(String(ssid));
    if (index < 0)
    {
        return false;
    }

    networkName = savedNetworkNames[index];
    networkPassword = savedNetworkPasswords[index];
    startupSelectionActive = false;
    saveAfterConnection = false;
    WiFi.disconnect(false, false);
    retryAt = millis() + CONNECTION_RESET_MS;
    connectionResetPending = true;
    setState(WifiState::Disconnected);
    return true;
}

bool WifiManager::forgetSavedNetwork(const char* ssid)
{
    if (ssid == nullptr)
    {
        return false;
    }

    const int removedIndex = findSavedNetwork(String(ssid));
    if (removedIndex < 0)
    {
        return false;
    }

    const bool forgetsCurrentNetwork = networkName == ssid;

    for (
        uint8_t index = removedIndex;
        index + 1 < savedNetworkCount;
        index++
    ) {
        savedNetworkNames[index] = savedNetworkNames[index + 1];
        savedNetworkPasswords[index] = savedNetworkPasswords[index + 1];
    }

    savedNetworkCount--;
    savedNetworkNames[savedNetworkCount] = "";
    savedNetworkPasswords[savedNetworkCount] = "";
    savedNetworkAvailable = savedNetworkCount > 0;
    persistSavedNetworks();

    if (forgetsCurrentNetwork)
    {
        disconnect();
        networkName = "";
        networkPassword = "";
    }

    Serial.print(F("Forgot Wi-Fi network: "));
    Serial.println(ssid);
    return true;
}

bool WifiManager::startSetupAccessPoint()
{
    if (setupAccessPointActive)
    {
        return true;
    }

    if (savedNetworkScanActive)
    {
        WiFi.scanDelete();
        savedNetworkScanActive = false;
        retryAt = 0;
        setState(WifiState::Disconnected);
    }
    if (savedNetworkScanPending)
    {
        savedNetworkScanPending = false;
        retryAt = 0;
        setState(WifiState::Disconnected);
    }

    WiFi.mode(isConnected() ? WIFI_AP_STA : WIFI_AP);

    if (!WiFi.softAP(SETUP_NETWORK_NAME, SETUP_NETWORK_PASSWORD))
    {
        Serial.println(F("Could not start Wi-Fi setup network"));
        return false;
    }

    setupAccessPointActive = true;
    Serial.print(F("Wi-Fi setup network: "));
    Serial.println(SETUP_NETWORK_NAME);
    Serial.print(F("Wi-Fi setup address: "));
    Serial.println(WiFi.softAPIP());
    return true;
}

void WifiManager::stopSetupAccessPoint()
{
    if (!setupAccessPointActive)
    {
        return;
    }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    setupAccessPointActive = false;
    Serial.println(F("Wi-Fi setup network stopped"));
}

void WifiManager::update()
{
    const unsigned long now = millis();

    if (savedNetworkScanActive)
    {
        updateSavedNetworkScan();
        return;
    }

    if (savedNetworkScanPending)
    {
        if (static_cast<long>(now - retryAt) >= 0)
        {
            savedNetworkScanPending = false;
            beginSavedNetworkScan();
        }
        return;
    }

    if (connectionResetPending)
    {
        if (static_cast<long>(now - retryAt) >= 0)
        {
            connectionResetPending = false;
            startConnection();
        }

        return;
    }

    const wl_status_t wifiStatus = WiFi.status();

    if (wifiStatus == WL_CONNECTED)
    {
        if (saveAfterConnection)
        {
            saveNetwork();
        }

        setState(WifiState::Connected);
        startupSelectionActive = false;
        visibleSavedNetworkCount = 0;
        nextVisibleSavedNetwork = 0;
        return;
    }

    if (state == WifiState::Connected)
    {
        visibleSavedNetworkCount = 0;
        nextVisibleSavedNetwork = 0;
        setState(WifiState::Disconnected);
        retryAt = 0;
        startupSelectionActive = false;
    }

    if (state == WifiState::Connecting)
    {
        const bool rejected =
            wifiStatus == WL_CONNECT_FAILED ||
            wifiStatus == WL_NO_SSID_AVAIL;

        if (rejected || now - connectionStartedAt >= CONNECTION_TIMEOUT_MS)
        {
            WiFi.disconnect(false, false);

            if (startupSelectionActive && startNextVisibleSavedNetwork())
            {
                return;
            }

            setState(WifiState::Failed);
            retryAt = 0;
            startupSelectionActive = false;
        }

        return;
    }

}

WifiState WifiManager::getState() const
{
    return state;
}

bool WifiManager::isConnected() const
{
    return state == WifiState::Connected;
}

bool WifiManager::hasSavedNetwork() const
{
    return savedNetworkAvailable;
}

uint8_t WifiManager::getSavedNetworkCount() const
{
    return savedNetworkCount;
}

const char* WifiManager::getSavedNetworkName(uint8_t index) const
{
    return index < savedNetworkCount
        ? savedNetworkNames[index].c_str()
        : "";
}

bool WifiManager::isSetupAccessPointActive() const
{
    return setupAccessPointActive;
}

const char* WifiManager::getNetworkName() const
{
    return networkName.c_str();
}

const char* WifiManager::getSetupNetworkName() const
{
    return SETUP_NETWORK_NAME;
}

const char* WifiManager::getSetupNetworkPassword() const
{
    return SETUP_NETWORK_PASSWORD;
}

String WifiManager::getSetupAddress() const
{
    return WiFi.softAPIP().toString();
}

void WifiManager::startConnection()
{
    WiFi.mode(setupAccessPointActive ? WIFI_AP_STA : WIFI_STA);
    WiFi.begin(networkName.c_str(), networkPassword.c_str());
    connectionStartedAt = millis();
    retryAt = 0;
    setState(WifiState::Connecting);
}

void WifiManager::startSavedNetworkScan()
{
    if (
        savedNetworkCount == 0 ||
        setupAccessPointActive ||
        savedNetworkScanActive ||
        savedNetworkScanPending
    ) {
        return;
    }

    WiFi.disconnect(false, false);
    savedNetworkScanPending = true;
    retryAt = millis() + CONNECTION_RESET_MS;
    setState(WifiState::Connecting);
}

void WifiManager::beginSavedNetworkScan()
{
    WiFi.mode(WIFI_STA);
    visibleSavedNetworkCount = 0;
    nextVisibleSavedNetwork = 0;
    WiFi.scanDelete();
    const int result = WiFi.scanNetworks(true, false);

    if (result == WIFI_SCAN_RUNNING || result >= 0)
    {
        savedNetworkScanActive = true;
        retryAt = 0;
        setState(WifiState::Connecting);
        Serial.println(F("Scanning for saved Wi-Fi networks"));
    }
    else
    {
        retryAt = 0;
        startupSelectionActive = false;
        setState(WifiState::Failed);
        Serial.println(F("Could not start saved-network scan"));
    }
}

bool WifiManager::updateSavedNetworkScan()
{
    const int result = WiFi.scanComplete();

    if (result == WIFI_SCAN_RUNNING)
    {
        return false;
    }

    savedNetworkScanActive = false;
    if (result >= 0)
    {
        collectVisibleSavedNetworks(result);
    }
    WiFi.scanDelete();

    if (!startNextVisibleSavedNetwork())
    {
        retryAt = 0;
        startupSelectionActive = false;
        setState(WifiState::Failed);
        Serial.println(F("No saved Wi-Fi networks are currently available"));
        return true;
    }

    return true;
}

void WifiManager::collectVisibleSavedNetworks(int scanResultCount)
{
    visibleSavedNetworkCount = 0;
    nextVisibleSavedNetwork = 0;

    for (uint8_t savedIndex = 0; savedIndex < savedNetworkCount; savedIndex++)
    {
        for (int scanIndex = 0; scanIndex < scanResultCount; scanIndex++)
        {
            if (savedNetworkNames[savedIndex] == WiFi.SSID(scanIndex))
            {
                visibleSavedNetworkIndices[visibleSavedNetworkCount++] =
                    savedIndex;
                break;
            }
        }
    }
}

bool WifiManager::startNextVisibleSavedNetwork()
{
    if (nextVisibleSavedNetwork >= visibleSavedNetworkCount)
    {
        return false;
    }

    const uint8_t savedIndex =
        visibleSavedNetworkIndices[nextVisibleSavedNetwork++];
    networkName = savedNetworkNames[savedIndex];
    networkPassword = savedNetworkPasswords[savedIndex];
    saveAfterConnection = false;
    Serial.print(F("Trying saved Wi-Fi network: "));
    Serial.println(networkName);
    startConnection();
    return true;
}

void WifiManager::saveNetwork()
{
    saveAfterConnection = false;

    if (!preferencesReady)
    {
        Serial.println(F("Wi-Fi preferences are unavailable"));
        return;
    }

    makeSavedNetworkPreferred(networkName, networkPassword);
    persistSavedNetworks();
    savedNetworkAvailable = true;
    Serial.print(F("Saved Wi-Fi network: "));
    Serial.println(networkName);
}

void WifiManager::loadSavedNetworks()
{
    savedNetworkCount = min(
        preferences.getUChar("netCount", 0),
        MAX_SAVED_NETWORKS
    );

    for (uint8_t index = 0; index < savedNetworkCount; index++)
    {
        char ssidKey[8];
        char passwordKey[8];
        snprintf(ssidKey, sizeof(ssidKey), "ssid%u", index);
        snprintf(passwordKey, sizeof(passwordKey), "pass%u", index);
        savedNetworkNames[index] = preferences.getString(ssidKey, "");
        savedNetworkPasswords[index] =
            preferences.getString(passwordKey, "");

        if (savedNetworkNames[index].length() == 0)
        {
            savedNetworkCount = index;
            break;
        }
    }

    if (savedNetworkCount > 0)
    {
        return;
    }

    const String legacyName = preferences.getString("ssid", "");
    if (legacyName.length() == 0)
    {
        return;
    }

    savedNetworkNames[0] = legacyName;
    savedNetworkPasswords[0] = preferences.getString("password", "");
    savedNetworkCount = 1;
    persistSavedNetworks();
    preferences.remove("ssid");
    preferences.remove("password");
    Serial.println(F("Migrated saved Wi-Fi network storage"));
}

void WifiManager::persistSavedNetworks()
{
    if (!preferencesReady)
    {
        return;
    }

    preferences.putUChar("netCount", savedNetworkCount);
    for (uint8_t index = 0; index < MAX_SAVED_NETWORKS; index++)
    {
        char ssidKey[8];
        char passwordKey[8];
        snprintf(ssidKey, sizeof(ssidKey), "ssid%u", index);
        snprintf(passwordKey, sizeof(passwordKey), "pass%u", index);

        if (index < savedNetworkCount)
        {
            preferences.putString(ssidKey, savedNetworkNames[index]);
            preferences.putString(
                passwordKey,
                savedNetworkPasswords[index]
            );
        }
        else
        {
            preferences.remove(ssidKey);
            preferences.remove(passwordKey);
        }
    }
}

int WifiManager::findSavedNetwork(const String& ssid) const
{
    for (uint8_t index = 0; index < savedNetworkCount; index++)
    {
        if (savedNetworkNames[index] == ssid)
        {
            return index;
        }
    }
    return -1;
}

void WifiManager::makeSavedNetworkPreferred(
    const String& ssid,
    const String& password
)
{
    const int existingIndex = findSavedNetwork(ssid);
    uint8_t insertionCount = savedNetworkCount;

    if (existingIndex < 0 && insertionCount < MAX_SAVED_NETWORKS)
    {
        insertionCount++;
    }

    const int shiftStart = existingIndex >= 0
        ? existingIndex
        : insertionCount - 1;
    for (int index = shiftStart; index > 0; index--)
    {
        savedNetworkNames[index] = savedNetworkNames[index - 1];
        savedNetworkPasswords[index] = savedNetworkPasswords[index - 1];
    }

    savedNetworkNames[0] = ssid;
    savedNetworkPasswords[0] = password;
    savedNetworkCount = insertionCount;
}

void WifiManager::setState(WifiState nextState)
{
    if (state == nextState)
    {
        return;
    }

    state = nextState;
    Serial.print(F("Wi-Fi state: "));
    Serial.println(getStateName(state));

    if (state == WifiState::Connected)
    {
        Serial.print(F("Wi-Fi network: "));
        Serial.println(networkName);
        Serial.print(F("Wi-Fi address: "));
        Serial.println(WiFi.localIP());
    }
}

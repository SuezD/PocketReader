#pragma once

#include <Arduino.h>
#include <Preferences.h>

enum class WifiState : uint8_t
{
    Disconnected,
    Connecting,
    Connected,
    Failed
};

class WifiManager
{
public:
    void begin();
    void connect(const char* ssid, const char* password);
    void disconnect();
    bool connectSavedNetwork(const char* ssid);
    bool forgetSavedNetwork(const char* ssid);
    bool startSetupAccessPoint();
    void stopSetupAccessPoint();
    void update();

    WifiState getState() const;
    bool isConnected() const;
    bool hasSavedNetwork() const;
    uint8_t getSavedNetworkCount() const;
    const char* getSavedNetworkName(uint8_t index) const;
    bool isSetupAccessPointActive() const;
    const char* getNetworkName() const;
    const char* getSetupNetworkName() const;
    const char* getSetupNetworkPassword() const;
    String getSetupAddress() const;

private:
    static constexpr uint8_t MAX_SAVED_NETWORKS = 5;
    static constexpr unsigned long CONNECTION_TIMEOUT_MS = 15000;
    static constexpr unsigned long CONNECTION_RESET_MS = 500;
    WifiState state = WifiState::Disconnected;
    Preferences preferences;
    String networkName;
    String networkPassword;
    String savedNetworkNames[MAX_SAVED_NETWORKS];
    String savedNetworkPasswords[MAX_SAVED_NETWORKS];
    uint8_t savedNetworkCount = 0;
    uint8_t visibleSavedNetworkIndices[MAX_SAVED_NETWORKS] = {};
    uint8_t visibleSavedNetworkCount = 0;
    uint8_t nextVisibleSavedNetwork = 0;
    unsigned long connectionStartedAt = 0;
    unsigned long retryAt = 0;
    bool startupSelectionActive = false;
    bool connectionResetPending = false;
    bool preferencesReady = false;
    bool savedNetworkAvailable = false;
    bool saveAfterConnection = false;
    bool setupAccessPointActive = false;
    bool savedNetworkScanActive = false;
    bool savedNetworkScanPending = false;

    void startConnection();
    void startSavedNetworkScan();
    void beginSavedNetworkScan();
    bool updateSavedNetworkScan();
    void collectVisibleSavedNetworks(int scanResultCount);
    bool startNextVisibleSavedNetwork();
    void saveNetwork();
    void loadSavedNetworks();
    void persistSavedNetworks();
    int findSavedNetwork(const String& ssid) const;
    void makeSavedNetworkPreferred(
        const String& ssid,
        const String& password
    );
    void setState(WifiState nextState);
};

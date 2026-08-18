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
    void forgetNetwork();
    void update();

    WifiState getState() const;
    bool isConnected() const;
    bool hasSavedNetwork() const;
    const char* getNetworkName() const;

private:
    static constexpr unsigned long CONNECTION_TIMEOUT_MS = 15000;
    static constexpr unsigned long RETRY_DELAY_MS = 10000;

    WifiState state = WifiState::Disconnected;
    Preferences preferences;
    String networkName;
    String networkPassword;
    unsigned long connectionStartedAt = 0;
    unsigned long retryAt = 0;
    bool shouldReconnect = false;
    bool preferencesReady = false;
    bool savedNetworkAvailable = false;
    bool saveAfterConnection = false;

    void startConnection();
    void saveNetwork();
    void setState(WifiState nextState);
};

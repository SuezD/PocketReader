#include "wifi/WifiProvisioningPortal.h"

#include "wifi/WifiService.h"

namespace
{
    constexpr char SETUP_PAGE[] = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Pocket Reader Wi-Fi</title>
  <style>
    body{font-family:sans-serif;max-width:28rem;margin:3rem auto;padding:0 1rem}
    label,input,button{display:block;width:100%;box-sizing:border-box}
    label{margin-top:1rem}input,button{font-size:1rem;padding:.75rem;margin-top:.35rem}
    button{margin-top:1.5rem}#status{margin-top:1rem}
  </style>
</head>
<body>
  <h1>Pocket Reader Wi-Fi</h1>
  <form method="post" action="/connect">
    <label for="ssid">Network name</label>
    <input id="ssid" name="ssid" maxlength="32" required autocomplete="off">
    <label for="password">Password</label>
    <input id="password" name="password" type="password" maxlength="63">
    <button type="submit">Connect</button>
  </form>
</body>
</html>
)HTML";

    constexpr char CONNECTING_PAGE[] = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Connecting</title>
  <style>
    body{font-family:sans-serif;max-width:28rem;margin:3rem auto;padding:0 1rem}
    #retry{display:none;margin-top:1.5rem;padding:.75rem;background:#222;color:#fff;text-decoration:none;text-align:center}
  </style>
</head>
<body>
  <h1>Connecting...</h1>
  <p id="status">Testing the network details.</p>
  <a id="retry" href="/">Try Again</a>
  <script>
    async function check(){
      try{
        const response=await fetch('/status',{cache:'no-store'});
        const result=await response.json();
        document.getElementById('status').textContent=result.message;
        if(result.state==='failed'){
          document.querySelector('h1').textContent='Connection Failed';
          document.getElementById('retry').style.display='block';
        }else if(result.state==='connected'){
          document.querySelector('h1').textContent='Connected';
        }else{
          setTimeout(check,1000);
        }
      }catch(error){}
    }
    setTimeout(check,500);
  </script>
</body>
</html>
)HTML";

    const char* getStatusName(WifiState state)
    {
        switch (state)
        {
            case WifiState::Disconnected: return "disconnected";
            case WifiState::Connecting: return "connecting";
            case WifiState::Connected: return "connected";
            case WifiState::Failed: return "failed";
        }

        return "unknown";
    }
}

WifiProvisioningPortal::WifiProvisioningPortal()
    : server(80)
{
}

bool WifiProvisioningPortal::start()
{
    if (active)
    {
        return true;
    }

    if (!getWifiManager().startSetupAccessPoint())
    {
        return false;
    }

    server.on("/", HTTP_GET, [this]() { handleHome(); });
    server.on("/connect", HTTP_POST, [this]() { handleConnect(); });
    server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server.onNotFound([this]() { handleNotFound(); });
    server.begin();
    active = true;
    connectionSubmitted = false;
    connectedAt = 0;
    Serial.println(F("Wi-Fi setup page started"));
    return true;
}

void WifiProvisioningPortal::stop()
{
    if (!active)
    {
        return;
    }

    server.stop();
    getWifiManager().stopSetupAccessPoint();
    active = false;
    connectionSubmitted = false;
    connectedAt = 0;
    Serial.println(F("Wi-Fi setup page stopped"));
}

void WifiProvisioningPortal::update()
{
    if (!active)
    {
        return;
    }

    server.handleClient();

    if (!connectionSubmitted || !getWifiManager().isConnected())
    {
        return;
    }

    if (connectedAt == 0)
    {
        connectedAt = millis();
    }
    else if (millis() - connectedAt >= SUCCESS_PAGE_DELAY_MS)
    {
        stop();
    }
}

bool WifiProvisioningPortal::isActive() const
{
    return active;
}

void WifiProvisioningPortal::handleHome()
{
    server.send(200, "text/html", SETUP_PAGE);
}

void WifiProvisioningPortal::handleConnect()
{
    if (!server.hasArg("ssid"))
    {
        server.send(400, "text/plain", "Network name is required");
        return;
    }

    const String ssid = server.arg("ssid");
    const String password = server.arg("password");

    if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 63)
    {
        server.send(400, "text/plain", "Invalid network details");
        return;
    }

    connectionSubmitted = true;
    connectedAt = 0;
    getWifiManager().connect(ssid.c_str(), password.c_str());
    server.send(202, "text/html", CONNECTING_PAGE);
}

void WifiProvisioningPortal::handleStatus()
{
    const WifiState state = getWifiManager().getState();
    String response = "{\"state\":\"";
    response += getStatusName(state);
    response += "\",\"message\":\"";

    if (state == WifiState::Connected)
    {
        response += "Connected. You can return to the reader.";
    }
    else if (state == WifiState::Failed)
    {
        response += "Connection failed. Check the network name and password.";
    }
    else
    {
        response += "Testing the network details...";
    }

    response += "\"}";
    server.send(200, "application/json", response);
}

void WifiProvisioningPortal::handleNotFound()
{
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

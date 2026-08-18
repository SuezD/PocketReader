#include <Arduino.h>

#include "Display.h"
#include "Input.h"
#include "books/BookCache.h"
#include "navigation/PageRegistry.h"
#include "screens/Startup.h"
#include "wifi/DevelopmentWifiConfig.h"
#include "wifi/WifiManager.h"

namespace
{
    constexpr uint8_t PAGE_STACK_CAPACITY = 8;
    constexpr unsigned long DOUBLE_TAP_MS = 350;
    constexpr uint8_t BATTERY_PERCENT = 85;

    WifiManager wifiManager;

    PageId pageStack[PAGE_STACK_CAPACITY] = { PageId::MainMenu };
    uint8_t pageStackSize = 1;
    bool startupReady = false;
    bool startupDismissed = false;
    bool selectTapPending = false;
    unsigned long firstSelectTapTime = 0;

    PageId getCurrentPageId()
    {
        return pageStack[pageStackSize - 1];
    }

    const PageDefinition* getCurrentPage()
    {
        return findPage(getCurrentPageId());
    }

    void performStartupTasks()
    {
        initBookCache();
        initializeRegisteredPages();
        wifiManager.begin();
        wifiManager.connect(
            DevelopmentWifi::SSID,
            DevelopmentWifi::PASSWORD
        );
        // Later: read battery level.
        startupReady = true;
    }

    void drawCurrentPage()
    {
        const PageDefinition* page = getCurrentPage();

        if (page == nullptr || page->page == nullptr)
        {
            Serial.println(F("Page is not registered"));
            return;
        }

        page->page->draw(BATTERY_PERCENT);
    }

    void enterCurrentPage()
    {
        const PageDefinition* page = getCurrentPage();

        if (page != nullptr && page->page != nullptr)
        {
            page->page->onEnter();
        }

        drawCurrentPage();
    }

    void exitCurrentPage()
    {
        const PageDefinition* page = getCurrentPage();

        if (page != nullptr && page->page != nullptr)
        {
            page->page->onExit();
        }
    }

    bool isRegistered(PageId destination)
    {
        if (findPage(destination) != nullptr)
        {
            return true;
        }

        Serial.println(F("Navigation destination is not registered"));
        return false;
    }

    void navigateTo(PageId destination)
    {
        if (!isRegistered(destination))
        {
            return;
        }

        if (pageStackSize >= PAGE_STACK_CAPACITY)
        {
            Serial.println(F("Page stack is full"));
            return;
        }

        exitCurrentPage();
        pageStack[pageStackSize++] = destination;
        enterCurrentPage();
    }

    void replaceCurrentPage(PageId destination)
    {
        if (!isRegistered(destination))
        {
            return;
        }

        exitCurrentPage();
        pageStack[pageStackSize - 1] = destination;
        enterCurrentPage();
    }

    void navigateHome(PageId destination)
    {
        if (!isRegistered(destination))
        {
            return;
        }

        exitCurrentPage();
        pageStack[0] = destination;
        pageStackSize = 1;
        enterCurrentPage();
    }

    void navigateBack()
    {
        if (pageStackSize <= 1)
        {
            return;
        }

        exitCurrentPage();
        pageStackSize--;
        enterCurrentPage();
    }

    void applyNavigation(const NavigationRequest& request)
    {
        switch (request.mode)
        {
            case NavigationMode::Push:
                navigateTo(request.destination);
                break;
            case NavigationMode::Replace:
                replaceCurrentPage(request.destination);
                break;
            case NavigationMode::Home:
                navigateHome(request.destination);
                break;
            case NavigationMode::None:
                break;
        }
    }

    void selectCurrentItem()
    {
        const PageDefinition* page = getCurrentPage();

        if (page != nullptr && page->page != nullptr)
        {
            applyNavigation(page->page->select());
        }
    }

    void handleSelectPress()
    {
        const unsigned long now = millis();

        if (selectTapPending && now - firstSelectTapTime <= DOUBLE_TAP_MS)
        {
            selectTapPending = false;
            navigateBack();
            return;
        }

        selectTapPending = true;
        firstSelectTapTime = now;
    }

    void processPendingSelectTap()
    {
        if (!selectTapPending || millis() - firstSelectTapTime < DOUBLE_TAP_MS)
        {
            return;
        }

        selectTapPending = false;
        selectCurrentItem();
    }

    void logInput(const InputState& input)
    {
        if (input.upPressed) Serial.println(F("Up pressed"));
        if (input.downPressed) Serial.println(F("Down pressed"));
        if (input.selectPressed) Serial.println(F("Select pressed"));
    }

    void handleStartupInput(const InputState& input)
    {
        if (
            startupReady &&
            (input.upPressed || input.downPressed || input.selectPressed)
        ) {
            startupDismissed = true;
            selectTapPending = false;
            enterCurrentPage();
        }
    }
}

void setup()
{
    Serial.begin(115200);
    initInput();
    initDisplay();
    drawStartupScreen(false);
    performStartupTasks();
    drawStartupScreen(true);
}

void loop()
{
    wifiManager.update();

    const InputState input = readInput();
    logInput(input);

    if (!startupDismissed)
    {
        handleStartupInput(input);
        return;
    }

    const PageDefinition* page = getCurrentPage();

    if (
        page != nullptr &&
        page->page != nullptr &&
        page->page->handleInput(input)
    ) {
        selectTapPending = false;
    }

    if (input.selectPressed)
    {
        handleSelectPress();
    }

    processPendingSelectTap();
}

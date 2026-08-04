#include <Arduino.h>

#include "Display.h"
#include "BoardConfig.h"
#include "screens/Startup.h"
#include "screens/MainMenu.h"

enum class Page
{
    Startup,
    MainMenu,
    Reader,
    MyBooks
};

constexpr uint8_t PAGE_STACK_CAPACITY = 8;

Page pageStack[PAGE_STACK_CAPACITY] = {
    Page::Startup
};

uint8_t pageStackSize = 1;

Page getCurrentPage()
{
    return pageStack[pageStackSize - 1];
}

constexpr unsigned long DEBOUNCE_MS = 30;

struct Button
{
    uint8_t pin;
    bool stableState;
    bool lastReading;
    unsigned long lastChangeTime;
};

Button previousButton = {
    BoardConfig::PREVIOUS_BUTTON_PIN,
    HIGH,
    HIGH,
    0
};

Button nextButton = {
    BoardConfig::NEXT_BUTTON_PIN,
    HIGH,
    HIGH,
    0
};

Button selectButton = {
    BoardConfig::SELECT_BUTTON_PIN,
    HIGH,
    HIGH,
    0
};

bool startupReady = false;

bool wasPressed(Button& button)
{
    const bool reading = digitalRead(button.pin);
    const unsigned long now = millis();

    if (reading != button.lastReading)
    {
        button.lastReading = reading;
        button.lastChangeTime = now;
    }

    if (
        now - button.lastChangeTime >= DEBOUNCE_MS &&
        reading != button.stableState
    )
    {
        button.stableState = reading;

        return button.stableState == LOW;
    }

    return false;
}

void performStartupTasks()
{
    // Later:
    // - Initialise SD card
    // - Find available books
    // - Restore reading progress
    // - Read battery level

    startupReady = true;
}

void drawCurrentPage()
{
    switch (getCurrentPage())
    {
        case Page::MainMenu:
            drawMainMenu(85);
            break;

        case Page::Reader:
            // drawReader();
            break;

        case Page::MyBooks:
            // drawMyBooks();
            break;

        case Page::Startup:
            break;
    }
}

void navigateTo(Page page)
{
    if (pageStackSize >= PAGE_STACK_CAPACITY)
    {
        Serial.println(F("Page stack is full"));
        return;
    }

    pageStack[pageStackSize] = page;
    pageStackSize++;

    drawCurrentPage();
}

void navigateBack()
{
    if (pageStackSize <= 1)
    {
        return;
    }

    pageStackSize--;
    drawCurrentPage();
}

void replaceCurrentPage(Page page)
{
    pageStack[pageStackSize - 1] = page;
    drawCurrentPage();
}

constexpr unsigned long DOUBLE_TAP_MS = 350;
bool selectTapPending = false;
unsigned long firstSelectTapTime = 0;

void selectCurrentItem()
{
    switch (getCurrentPage())
    {
        case Page::MainMenu:
            switch (getSelectedMainMenuItem())
            {
                case MainMenuItem::ContinueReading:
                    navigateTo(Page::Reader);
                    break;

                case MainMenuItem::MyBooks:
                    navigateTo(Page::MyBooks);
                    break;
            }
            break;

        case Page::MyBooks:
            // Later: open the selected book.
            break;

        default:
            break;
    }
}

void handleSelectPress()
{
    const unsigned long now = millis();

    if (
        selectTapPending &&
        now - firstSelectTapTime <= DOUBLE_TAP_MS
    ) {
        selectTapPending = false;
        navigateBack();
        return;
    }

    selectTapPending = true;
    firstSelectTapTime = now;
}

void processPendingSelectTap()
{
    if (
        !selectTapPending ||
        millis() - firstSelectTapTime < DOUBLE_TAP_MS
    ) {
        return;
    }

    selectTapPending = false;
    selectCurrentItem();
}

void setup()
{
    Serial.begin(115200);

    pinMode(BoardConfig::PREVIOUS_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BoardConfig::NEXT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BoardConfig::SELECT_BUTTON_PIN, INPUT_PULLUP);

    initDisplay();

    drawStartupScreen(false);
    performStartupTasks();
    drawStartupScreen(true);
}

void loop()
{
    const bool upPressed =
        wasPressed(previousButton);

    const bool downPressed =
        wasPressed(nextButton);

    const bool selectPressed =
        wasPressed(selectButton);

    if (upPressed)
    {
        Serial.println(F("Up pressed"));
    }

    if (downPressed)
    {
        Serial.println(F("Down pressed"));
    }

    if (selectPressed)
    {
        Serial.println(F("Select pressed"));
    }

    if (getCurrentPage() == Page::Startup)
    {
        if (
            startupReady &&
            (
                upPressed ||
                downPressed ||
                selectPressed
            )
        ) {
            selectTapPending = false;
            replaceCurrentPage(Page::MainMenu);
        }

        return;
    }

    if (getCurrentPage() == Page::MainMenu)
    {
        if (upPressed && !downPressed)
        {
            selectTapPending = false;
            moveMainMenuUp();
            redrawMainMenuList();
        }
        else if (downPressed && !upPressed)
        {
            selectTapPending = false;
            moveMainMenuDown();
            redrawMainMenuList();
        }
    }

    if (selectPressed)
    {
        handleSelectPress();
    }

    processPendingSelectTap();
}
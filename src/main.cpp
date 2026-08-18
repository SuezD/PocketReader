#include <Arduino.h>

#include "Display.h"
#include "Input.h"
#include "screens/MyBooks.h"
#include "screens/MainMenu.h"
#include "screens/Reader.h"
#include "screens/Startup.h"

namespace
{
    enum class Page
    {
        Startup,
        MainMenu,
        Reader,
        MyBooks
    };

    constexpr uint8_t PAGE_STACK_CAPACITY = 8;
    constexpr unsigned long DOUBLE_TAP_MS = 350;

    Page pageStack[PAGE_STACK_CAPACITY] = {
        Page::Startup
    };

    uint8_t pageStackSize = 1;
    bool startupReady = false;
    bool selectTapPending = false;
    unsigned long firstSelectTapTime = 0;

    Page getCurrentPage()
    {
        return pageStack[pageStackSize - 1];
    }

    void performStartupTasks()
    {
#if defined(ARDUINO_ARCH_ESP32)
        initBookCache();
#endif

        // Later:
        // - Find available books from the cache manifest
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
                drawReader(85);
                break;

            case Page::MyBooks:
                drawMyBooks(85);
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

        if (page == Page::Reader)
        {
            requestReaderFullRefresh();
        }

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

                    default:
                        break;
                }
                break;

            case Page::MyBooks:
            {
                const CachedBook& book = getSelectedMyBook();
                ReaderDocument document = {};
                openCachedBookDocument(book, document);
                openReader(
                    &book,
                    document,
                    getCachedBookPage(book)
                );
                navigateTo(Page::Reader);
                break;
            }

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

    void logInput(const InputState& input)
    {
        if (input.upPressed)
        {
            Serial.println(F("Up pressed"));
        }

        if (input.downPressed)
        {
            Serial.println(F("Down pressed"));
        }

        if (input.selectPressed)
        {
            Serial.println(F("Select pressed"));
        }
    }

    void handleStartupInput(const InputState& input)
    {
        if (
            startupReady &&
            (
                input.upPressed ||
                input.downPressed ||
                input.selectPressed
            )
        ) {
            selectTapPending = false;
            replaceCurrentPage(Page::MainMenu);
        }
    }

    void handleMainMenuInput(const InputState& input)
    {
        if (input.upPressed && !input.downPressed)
        {
            selectTapPending = false;
            const MainMenuItem previousItem = getSelectedMainMenuItem();

            if (moveMainMenuUp())
            {
                redrawMainMenuSelection(previousItem);
            }
        }
        else if (input.downPressed && !input.upPressed)
        {
            selectTapPending = false;
            const MainMenuItem previousItem = getSelectedMainMenuItem();

            if (moveMainMenuDown())
            {
                redrawMainMenuSelection(previousItem);
            }
        }
    }

    void handleMyBooksInput(const InputState& input)
    {
        if (input.upPressed && !input.downPressed)
        {
            selectTapPending = false;
            const uint8_t previousIndex =
                getSelectedMyBookIndex();

            if (moveMyBooksUp())
            {
                redrawMyBooksSelection(previousIndex);
            }
        }
        else if (input.downPressed && !input.upPressed)
        {
            selectTapPending = false;
            const uint8_t previousIndex =
                getSelectedMyBookIndex();

            if (moveMyBooksDown())
            {
                redrawMyBooksSelection(previousIndex);
            }
        }
    }

    void handleReaderInput(const InputState& input)
    {
        if (input.upPressed && !input.downPressed)
        {
            selectTapPending = false;

            if (moveReaderPreviousPage())
            {
                drawReader(85);
            }
        }
        else if (input.downPressed && !input.upPressed)
        {
            selectTapPending = false;

            if (moveReaderNextPage())
            {
                drawReader(85);
            }
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
    const InputState input = readInput();
    logInput(input);

    if (getCurrentPage() == Page::Startup)
    {
        handleStartupInput(input);
        return;
    }

    if (getCurrentPage() == Page::MainMenu)
    {
        handleMainMenuInput(input);
    }

    if (getCurrentPage() == Page::MyBooks)
    {
        handleMyBooksInput(input);
    }

    if (getCurrentPage() == Page::Reader)
    {
        handleReaderInput(input);
    }

    if (input.selectPressed)
    {
        handleSelectPress();
    }

    processPendingSelectTap();
}

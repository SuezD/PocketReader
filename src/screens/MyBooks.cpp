#include "screens/MyBooks.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"
#include "navigation/PageRegistry.h"
#include "screens/Reader.h"

namespace
{
    void drawSelectionMarker(uint8_t selectedIndex)
    {
        const int itemStride =
            Theme::SELECT_LIST_ITEM_HEIGHT +
            Theme::SELECT_LIST_ITEM_GAP;

        const int rowY =
            Theme::HEADER_HEIGHT +
            1 +
            Theme::SELECT_LIST_VERTICAL_PADDING +
            selectedIndex * itemStride;

        display.fillRect(
            Theme::PAGE_MARGIN,
            rowY,
            Theme::SELECT_LIST_MARKER_WIDTH,
            Theme::SELECT_LIST_ITEM_HEIGHT,
            Theme::TEXT_COLOR
        );
    }
}

MyBooksPage::MyBooksPage(ReaderPage& readerPage)
    : readerPage(readerPage)
{
}

void MyBooksPage::draw(uint8_t batteryPercent)
{
    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("MY BOOKS", batteryPercent);

        if (getCachedBookCount() == 0)
        {
            const char* options[MAX_NAVIGATION_OPTIONS];
            getNavigationOptionLabels(
                MY_BOOKS_EMPTY_OPTIONS,
                options,
                MY_BOOKS_EMPTY_OPTION_COUNT
            );

            drawMessage(
                "No Books Downloaded",
                nullptr,
                options,
                MY_BOOKS_EMPTY_OPTION_COUNT,
                0
            );
        }
        else
        {
            drawSelectList(
                getCachedBookTitles(),
                getCachedBookCount(),
                listState
            );
        }

        drawFooter();
    }
    while (display.nextPage());
}

void MyBooksPage::redrawSelection(uint8_t previousIndex)
{
    if (getCachedBookCount() == 0)
    {
        return;
    }

    const uint8_t selectedIndex = listState.selectedIndex;
    const uint8_t firstChangedIndex =
        min(previousIndex, selectedIndex);
    const uint8_t lastChangedIndex =
        max(previousIndex, selectedIndex);

    const int itemStride =
        Theme::SELECT_LIST_ITEM_HEIGHT +
        Theme::SELECT_LIST_ITEM_GAP;

    const int updateTop =
        Theme::HEADER_HEIGHT +
        1 +
        Theme::SELECT_LIST_VERTICAL_PADDING +
        firstChangedIndex * itemStride;

    const int updateHeight =
        (lastChangedIndex - firstChangedIndex) * itemStride +
        Theme::SELECT_LIST_ITEM_HEIGHT;

    display.setPartialWindow(
        Theme::PAGE_MARGIN,
        updateTop,
        Theme::SELECT_LIST_MARKER_WIDTH,
        updateHeight
    );

    display.firstPage();

    do
    {
        display.fillRect(
            Theme::PAGE_MARGIN,
            updateTop,
            Theme::SELECT_LIST_MARKER_WIDTH,
            updateHeight,
            Theme::BACKGROUND_COLOR
        );

        drawSelectionMarker(selectedIndex);
    }
    while (display.nextPage());
}

bool MyBooksPage::handleInput(const InputState& input)
{
    const uint8_t previousIndex = listState.selectedIndex;

    if (input.upPressed && !input.downPressed)
    {
        moveSelectListUp(listState, getCachedBookCount());
    }
    else if (input.downPressed && !input.upPressed)
    {
        moveSelectListDown(listState, getCachedBookCount());
    }
    else
    {
        return false;
    }

    if (listState.selectedIndex != previousIndex)
    {
        redrawSelection(previousIndex);
    }

    return true;
}

NavigationRequest MyBooksPage::select()
{
    if (getCachedBookCount() == 0)
    {
        return MY_BOOKS_EMPTY_OPTIONS[0];
    }

    const CachedBook& book = getCachedBook(listState.selectedIndex);
    ReaderDocument document = {};

    if (!openCachedBookDocument(book, document))
    {
        Serial.println(F("Could not open selected book"));
        return noNavigation();
    }

    readerPage.open(&book, document, getCachedBookPage(book));
    return { NavigationMode::Push, PageId::ContinueReading };
}

#include "screens/MyBooks.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"
#include "navigation/PageRegistry.h"
#include "screens/Reader.h"

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

bool MyBooksPage::handleInput(const InputState& input)
{
    const SelectListState previousState = listState;

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

    redrawSelectListAfterMove(
        getCachedBookTitles(),
        getCachedBookCount(),
        previousState,
        listState
    );

    return true;
}

NavigationRequest MyBooksPage::select()
{
    if (getCachedBookCount() == 0)
    {
        return MY_BOOKS_EMPTY_OPTIONS[0];
    }

    const CachedBook& book = getCachedBook(listState.selectedIndex);

    if (!readerPage.open(&book, getCachedBookPage(book)))
    {
        Serial.println(F("Could not open selected book"));
        return noNavigation();
    }

    return { NavigationMode::Push, PageId::ContinueReading };
}

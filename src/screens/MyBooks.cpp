#include "screens/MyBooks.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"
#include "navigation/PageRegistry.h"

MyBooksPage::MyBooksPage(SelectedCachedBook& nextSelectedBook)
    : selectedBook(nextSelectedBook)
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
                selectedEmptyOption
            );
        }
        else
        {
            drawSelectList(
                getCachedBookTitles(),
                getCachedBookCount(),
                listState,
                TWO_LINE_SELECT_LIST
            );
        }

        drawFooter(selectedBook.getStatus());
    }
    while (display.nextPage());
}

bool MyBooksPage::handleInput(const InputState& input)
{
    if (getCachedBookCount() == 0)
    {
        const uint8_t previousIndex = selectedEmptyOption;

        if (input.upPressed && !input.downPressed && selectedEmptyOption > 0)
        {
            selectedEmptyOption--;
        }
        else if (
            input.downPressed && !input.upPressed &&
            selectedEmptyOption + 1 < MY_BOOKS_EMPTY_OPTION_COUNT
        ) {
            selectedEmptyOption++;
        }
        else
        {
            return input.upPressed || input.downPressed;
        }

        const char* options[MAX_NAVIGATION_OPTIONS];
        getNavigationOptionLabels(
            MY_BOOKS_EMPTY_OPTIONS,
            options,
            MY_BOOKS_EMPTY_OPTION_COUNT
        );
        redrawMessageSelection(
            "No Books Downloaded",
            nullptr,
            options,
            MY_BOOKS_EMPTY_OPTION_COUNT,
            previousIndex,
            selectedEmptyOption
        );
        return true;
    }

    const SelectListState previousState = listState;

    if (input.upPressed && !input.downPressed)
    {
        moveSelectListUp(
            listState,
            getCachedBookCount()
        );
    }
    else if (input.downPressed && !input.upPressed)
    {
        moveSelectListDown(
            listState,
            getCachedBookCount()
        );
    }
    else
    {
        return false;
    }

    redrawSelectListAfterMove(
        getCachedBookTitles(),
        getCachedBookCount(),
        previousState,
        listState,
        TWO_LINE_SELECT_LIST
    );

    return true;
}

NavigationRequest MyBooksPage::select()
{
    if (getCachedBookCount() == 0)
    {
        return MY_BOOKS_EMPTY_OPTIONS[selectedEmptyOption];
    }

    selectedBook.select(getCachedBook(listState.selectedIndex));
    return { NavigationMode::Push, PageId::BookActions };
}

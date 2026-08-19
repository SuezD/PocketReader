#include "screens/MyBooks.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"
#include "components/Selection.h"
#include "navigation/PageRegistry.h"

namespace
{
    constexpr const char* MY_BOOKS_BOTTOM_ACTIONS[] = { "Back" };
}

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
            const char* items[MAX_BOOK_ITEMS];
            const uint8_t itemCount = getItems(items);
            drawSelectList(
                items,
                itemCount,
                listState,
                TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTION,
                MY_BOOKS_BOTTOM_ACTIONS
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

        if (!moveSelection(
            input,
            selectedEmptyOption,
            MY_BOOKS_EMPTY_OPTION_COUNT
        ))
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
    const char* items[MAX_BOOK_ITEMS];
    const uint8_t itemCount = getItems(items);

    if (input.upPressed && !input.downPressed)
    {
        moveSelectListUp(listState, itemCount + 1);
    }
    else if (input.downPressed && !input.upPressed)
    {
        moveSelectListDown(listState, itemCount + 1);
    }
    else
    {
        return false;
    }

    redrawSelectListAfterMove(
        items,
        getCachedBookCount(),
        previousState,
        listState,
        TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTION,
        MY_BOOKS_BOTTOM_ACTIONS
    );

    return true;
}

NavigationRequest MyBooksPage::select()
{
    if (getCachedBookCount() == 0)
    {
        return MY_BOOKS_EMPTY_OPTIONS[selectedEmptyOption];
    }

    if (listState.selectedIndex >= getCachedBookCount())
    {
        return { NavigationMode::Pop, PageId::MainMenu };
    }

    selectedBook.select(getCachedBook(listState.selectedIndex));
    return { NavigationMode::Push, PageId::BookActions };
}

uint8_t MyBooksPage::getItems(const char** items) const
{
    const uint8_t bookCount = getCachedBookCount();
    const char* const* titles = getCachedBookTitles();
    for (uint8_t index = 0; index < bookCount; index++)
    {
        items[index] = titles[index];
    }
    return bookCount;
}

#include "screens/MyBooks.h"

#include <stdio.h>

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"
#include "components/Selection.h"
#include "helpers/StorageText.h"
#include "navigation/PageRegistry.h"

namespace
{
    constexpr const char* MY_BOOKS_BOTTOM_ACTIONS[] = {
        "Add Books", "Back"
    };
}

MyBooksPage::MyBooksPage(SelectedCachedBook& nextSelectedBook)
    : selectedBook(nextSelectedBook)
{
}

void MyBooksPage::draw(uint8_t batteryPercent)
{
    char leftFooter[32];
    char rightFooter[32];
    getFooterText(
        leftFooter, sizeof(leftFooter),
        rightFooter, sizeof(rightFooter)
    );
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
                TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTIONS,
                MY_BOOKS_BOTTOM_ACTIONS
            );
        }

        drawFooter(
            leftFooter[0] == '\0' ? nullptr : leftFooter,
            rightFooter
        );
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
        moveSelectListUp(listState, itemCount + 2);
    }
    else if (input.downPressed && !input.upPressed)
    {
        moveSelectListDown(listState, itemCount + 2);
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
        TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTIONS,
        MY_BOOKS_BOTTOM_ACTIONS
    );
    selectedBook.setStatus("");
    char leftFooter[32];
    char rightFooter[32];
    getFooterText(
        leftFooter, sizeof(leftFooter),
        rightFooter, sizeof(rightFooter)
    );
    redrawFooter(
        leftFooter[0] == '\0' ? nullptr : leftFooter,
        rightFooter
    );

    return true;
}

NavigationRequest MyBooksPage::select()
{
    if (getCachedBookCount() == 0)
    {
        return MY_BOOKS_EMPTY_OPTIONS[selectedEmptyOption];
    }

    if (listState.selectedIndex == getCachedBookCount())
    {
        return navigateTo(PageId::AddBooks);
    }

    if (listState.selectedIndex > getCachedBookCount())
    {
        return navigateBack();
    }

    selectedBook.select(getCachedBook(listState.selectedIndex));
    return navigateTo(PageId::BookActions);
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

void MyBooksPage::getFooterText(
    char* leftText,
    size_t leftTextSize,
    char* rightText,
    size_t rightTextSize
) const {
    leftText[0] = '\0';
    const char* status = selectedBook.getStatus();
    if (status[0] != '\0')
    {
        snprintf(leftText, leftTextSize, "%s", status);
    }
    else if (
        getCachedBookCount() > 0 &&
        listState.selectedIndex < getCachedBookCount()
    ) {
        char sizeText[16];
        formatStorageSize(
            getCachedBookSizeBytes(getCachedBook(listState.selectedIndex)),
            sizeText,
            sizeof(sizeText)
        );
        snprintf(leftText, leftTextSize, "%s", sizeText);
    }

    char freeText[16];
    formatStorageSize(
        getAvailableBookStorageBytes(),
        freeText,
        sizeof(freeText)
    );
    snprintf(rightText, rightTextSize, "Free: %s", freeText);
}

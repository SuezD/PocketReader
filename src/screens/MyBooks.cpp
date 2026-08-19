#include "screens/MyBooks.h"

#include <stdio.h>

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "components/SelectList.h"
#include "components/Selection.h"
#include "helpers/StorageText.h"
#include "navigation/PageRegistry.h"

namespace
{
    constexpr const char* MY_BOOKS_BOTTOM_ACTIONS[] = {
        "Add Books", "Back"
    };
    constexpr const char* BOOK_ACTIONS[] = { "Read", "Delete", "Back" };
    constexpr uint8_t BOOK_ACTION_COUNT = 3;
    constexpr const char* DELETE_OPTIONS[] = { "Cancel", "Delete" };
    constexpr uint8_t DELETE_OPTION_COUNT = 2;
}

MyBooksPage::MyBooksPage(ReaderPage& nextReaderPage)
    : readerPage(nextReaderPage)
{
}

void MyBooksPage::onEnter()
{
    state = State::Catalogue;
    optionSelection = 0;
}

void MyBooksPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("MY BOOKS", batteryPercent);
        drawCurrentContent();

        char leftFooter[32];
        char rightFooter[32];
        getFooterText(
            leftFooter, sizeof(leftFooter),
            rightFooter, sizeof(rightFooter)
        );
        drawFooter(leftFooter[0] == '\0' ? nullptr : leftFooter, rightFooter);
    }
    while (display.nextPage());
}

bool MyBooksPage::handleInput(const InputState& input)
{
    if (state != State::Catalogue)
    {
        const uint8_t previousIndex = optionSelection;
        const uint8_t count = state == State::BookActions
            ? BOOK_ACTION_COUNT
            : DELETE_OPTION_COUNT;
        if (!moveSelection(input, optionSelection, count)) return false;
        if (optionSelection != previousIndex)
        {
            redrawCurrentSelection(previousIndex);
        }
        return true;
    }

    if (getCachedBookCount() == 0)
    {
        const uint8_t previousIndex = selectedEmptyOption;
        if (!moveSelection(
            input,
            selectedEmptyOption,
            MY_BOOKS_EMPTY_OPTION_COUNT
        )) {
            return input.upPressed || input.downPressed;
        }

        const char* options[MAX_NAVIGATION_OPTIONS];
        getNavigationOptionLabels(
            MY_BOOKS_EMPTY_OPTIONS,
            options,
            MY_BOOKS_EMPTY_OPTION_COUNT
        );
        redrawMessageSelection(
            "No Books Downloaded", nullptr,
            options, MY_BOOKS_EMPTY_OPTION_COUNT,
            previousIndex, selectedEmptyOption
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
        itemCount,
        previousState,
        listState,
        TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTIONS,
        MY_BOOKS_BOTTOM_ACTIONS
    );
    setStatus("");

    char leftFooter[32];
    char rightFooter[32];
    getFooterText(
        leftFooter, sizeof(leftFooter),
        rightFooter, sizeof(rightFooter)
    );
    redrawFooter(leftFooter[0] == '\0' ? nullptr : leftFooter, rightFooter);
    return true;
}

NavigationRequest MyBooksPage::select()
{
    if (state == State::Catalogue)
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

        selectBook(getCachedBook(listState.selectedIndex));
        enterState(State::BookActions);
        return noNavigation();
    }

    if (state == State::BookActions)
    {
        if (optionSelection == 0)
        {
            const CachedBook* book = findCachedBook(selectedBookId);
            if (book == nullptr || !readerPage.open(book, getCachedBookPage(*book)))
            {
                setStatus("Could not open book");
                enterState(State::Catalogue);
                return noNavigation();
            }
            return navigateTo(PageId::ContinueReading);
        }
        if (optionSelection == 1)
        {
            enterState(State::DeleteConfirmation);
            return noNavigation();
        }

        enterState(State::Catalogue);
        return noNavigation();
    }

    if (optionSelection == 0)
    {
        enterState(State::BookActions);
        return noNavigation();
    }

    readerPage.closeBook(selectedBookId);
    const BookDeleteResult result = deleteCachedBook(selectedBookId);
    setStatus(getBookDeleteResultText(result));
    if (result != BookDeleteResult::ManifestError)
    {
        selectedBookId[0] = '\0';
        selectedBookTitle[0] = '\0';
    }
    enterState(State::Catalogue);
    return noNavigation();
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
    if (status[0] != '\0')
    {
        snprintf(leftText, leftTextSize, "%s", status);
    }
    else if (
        state == State::Catalogue &&
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

void MyBooksPage::selectBook(const CachedBook& book)
{
    snprintf(selectedBookId, sizeof(selectedBookId), "%s", book.id);
    snprintf(selectedBookTitle, sizeof(selectedBookTitle), "%s", book.title);
    setStatus("");
}

void MyBooksPage::setStatus(const char* nextStatus)
{
    snprintf(status, sizeof(status), "%s", nextStatus);
}

void MyBooksPage::drawBody()
{
    setPageBodyPartialWindow();
    display.firstPage();
    do
    {
        clearPageBody();
        drawCurrentContent();

        char leftFooter[32];
        char rightFooter[32];
        getFooterText(
            leftFooter, sizeof(leftFooter),
            rightFooter, sizeof(rightFooter)
        );
        drawFooter(leftFooter[0] == '\0' ? nullptr : leftFooter, rightFooter);
    }
    while (display.nextPage());
}

void MyBooksPage::drawCurrentContent()
{
    if (state == State::BookActions)
    {
        drawMessage(
            selectedBookTitle, nullptr,
            BOOK_ACTIONS, BOOK_ACTION_COUNT, optionSelection
        );
        return;
    }
    if (state == State::DeleteConfirmation)
    {
        drawMessage(
            selectedBookTitle, nullptr,
            DELETE_OPTIONS, DELETE_OPTION_COUNT, optionSelection
        );
        return;
    }

    if (getCachedBookCount() == 0)
    {
        const char* options[MAX_NAVIGATION_OPTIONS];
        getNavigationOptionLabels(
            MY_BOOKS_EMPTY_OPTIONS,
            options,
            MY_BOOKS_EMPTY_OPTION_COUNT
        );
        drawMessage(
            "No Books Downloaded", nullptr,
            options, MY_BOOKS_EMPTY_OPTION_COUNT, selectedEmptyOption
        );
        return;
    }

    const char* items[MAX_BOOK_ITEMS];
    const uint8_t itemCount = getItems(items);
    if (listState.selectedIndex >= itemCount + 2)
    {
        listState.selectedIndex = itemCount + 1;
    }
    drawSelectList(
        items,
        itemCount,
        listState,
        TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTIONS,
        MY_BOOKS_BOTTOM_ACTIONS
    );
}

void MyBooksPage::redrawCurrentSelection(uint8_t previousIndex)
{
    if (state == State::BookActions)
    {
        redrawMessageSelection(
            selectedBookTitle, nullptr,
            BOOK_ACTIONS, BOOK_ACTION_COUNT,
            previousIndex, optionSelection
        );
    }
    else if (state == State::DeleteConfirmation)
    {
        redrawMessageSelection(
            selectedBookTitle, nullptr,
            DELETE_OPTIONS, DELETE_OPTION_COUNT,
            previousIndex, optionSelection
        );
    }
}

void MyBooksPage::enterState(State nextState)
{
    state = nextState;
    optionSelection = 0;
    drawBody();
}

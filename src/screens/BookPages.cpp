#include "screens/BookPages.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/Selection.h"

namespace
{
    constexpr const char* BOOK_ACTIONS[] = { "Read", "Delete", "Back" };
    constexpr uint8_t BOOK_ACTION_COUNT = 3;
    constexpr const char* DELETE_OPTIONS[] = { "Cancel", "Delete" };
    constexpr uint8_t DELETE_OPTION_COUNT = 2;

    void drawOptions(
        const char* heading,
        const char* message,
        const char* const options[],
        uint8_t optionCount,
        uint8_t selectedIndex,
        uint8_t batteryPercent
    ) {
        display.setFullWindow();
        display.firstPage();
        do
        {
            display.fillScreen(Theme::BACKGROUND_COLOR);
            drawHeader(heading, batteryPercent);
            drawMessage(message, nullptr, options, optionCount, selectedIndex);
            drawFooter();
        }
        while (display.nextPage());
    }
}

BookActionsPage::BookActionsPage(
    SelectedCachedBook& nextSelectedBook,
    ReaderPage& nextReaderPage
) : selectedBook(nextSelectedBook), readerPage(nextReaderPage)
{
}

void BookActionsPage::draw(uint8_t batteryPercent)
{
    selectedIndex = 0;
    drawOptions(
        "BOOK ACTIONS", selectedBook.getTitle(),
        BOOK_ACTIONS, BOOK_ACTION_COUNT, selectedIndex, batteryPercent
    );
}

bool BookActionsPage::handleInput(const InputState& input)
{
    const uint8_t previousIndex = selectedIndex;
    if (!moveSelection(input, selectedIndex, BOOK_ACTION_COUNT))
    {
        return input.upPressed || input.downPressed;
    }
    redrawMessageSelection(
        selectedBook.getTitle(), nullptr,
        BOOK_ACTIONS, BOOK_ACTION_COUNT, previousIndex, selectedIndex
    );
    return true;
}

NavigationRequest BookActionsPage::select()
{
    if (selectedIndex == 0)
    {
        const CachedBook* book = findCachedBook(selectedBook.getId());
        if (book == nullptr || !readerPage.open(book, getCachedBookPage(*book)))
        {
            selectedBook.setStatus("Could not open book");
            return { NavigationMode::PopTo, PageId::MyBooks };
        }
        return { NavigationMode::Push, PageId::ContinueReading };
    }
    if (selectedIndex == 1)
    {
        return { NavigationMode::Push, PageId::DeleteBook };
    }
    return { NavigationMode::Pop, PageId::MyBooks };
}

DeleteBookPage::DeleteBookPage(
    SelectedCachedBook& nextSelectedBook,
    ReaderPage& nextReaderPage
) : selectedBook(nextSelectedBook), readerPage(nextReaderPage)
{
}

void DeleteBookPage::draw(uint8_t batteryPercent)
{
    selectedIndex = 0;
    drawOptions(
        "DELETE BOOK", selectedBook.getTitle(),
        DELETE_OPTIONS, DELETE_OPTION_COUNT, selectedIndex, batteryPercent
    );
}

bool DeleteBookPage::handleInput(const InputState& input)
{
    const uint8_t previousIndex = selectedIndex;
    if (!moveSelection(input, selectedIndex, DELETE_OPTION_COUNT))
    {
        return input.upPressed || input.downPressed;
    }
    redrawMessageSelection(
        selectedBook.getTitle(), nullptr,
        DELETE_OPTIONS, DELETE_OPTION_COUNT, previousIndex, selectedIndex
    );
    return true;
}

NavigationRequest DeleteBookPage::select()
{
    if (selectedIndex == 0)
    {
        return { NavigationMode::Pop, PageId::BookActions };
    }

    readerPage.closeBook(selectedBook.getId());
    const BookDeleteResult result = deleteCachedBook(selectedBook.getId());
    selectedBook.setStatus(getBookDeleteResultText(result));
    if (result != BookDeleteResult::ManifestError)
    {
        selectedBook.clear();
        selectedBook.setStatus(getBookDeleteResultText(result));
    }
    return { NavigationMode::PopTo, PageId::MyBooks };
}

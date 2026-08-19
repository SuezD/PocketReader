#include "screens/BookPages.h"

namespace
{
    constexpr const char* BOOK_ACTIONS[] = { "Read", "Delete", "Back" };
    constexpr uint8_t BOOK_ACTION_COUNT = 3;
    constexpr const char* DELETE_OPTIONS[] = { "Cancel", "Delete" };
    constexpr uint8_t DELETE_OPTION_COUNT = 2;

}

BookActionsPage::BookActionsPage(
    SelectedCachedBook& nextSelectedBook,
    ReaderPage& nextReaderPage
) : selectedBook(nextSelectedBook), readerPage(nextReaderPage)
{
}

void BookActionsPage::draw(uint8_t batteryPercent)
{
    optionsPage.draw(
        "BOOK ACTIONS", selectedBook.getTitle(),
        nullptr, BOOK_ACTIONS, BOOK_ACTION_COUNT, batteryPercent
    );
}

bool BookActionsPage::handleInput(const InputState& input)
{
    return optionsPage.handleInput(
        input,
        selectedBook.getTitle(), nullptr,
        BOOK_ACTIONS, BOOK_ACTION_COUNT
    );
}

NavigationRequest BookActionsPage::select()
{
    if (optionsPage.selectedIndex() == 0)
    {
        const CachedBook* book = findCachedBook(selectedBook.getId());
        if (book == nullptr || !readerPage.open(book, getCachedBookPage(*book)))
        {
            selectedBook.setStatus("Could not open book");
            return { NavigationMode::PopTo, PageId::MyBooks };
        }
        return { NavigationMode::Push, PageId::ContinueReading };
    }
    if (optionsPage.selectedIndex() == 1)
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
    optionsPage.draw(
        "DELETE BOOK", selectedBook.getTitle(),
        nullptr, DELETE_OPTIONS, DELETE_OPTION_COUNT, batteryPercent
    );
}

bool DeleteBookPage::handleInput(const InputState& input)
{
    return optionsPage.handleInput(
        input,
        selectedBook.getTitle(), nullptr,
        DELETE_OPTIONS, DELETE_OPTION_COUNT
    );
}

NavigationRequest DeleteBookPage::select()
{
    if (optionsPage.selectedIndex() == 0)
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

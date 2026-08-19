#include "screens/AddBooks.h"

#include "Display.h"
#include "Theme.h"
#include "books/BookSync.h"
#include "books/BookCache.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "components/SelectList.h"
#include "components/Selection.h"
#include "navigation/PageRegistry.h"

namespace
{
    constexpr const char* DOWNLOAD_COMPLETE_OPTIONS[] = {
        "Read", "Back to Catalogue", "Main Menu"
    };
    constexpr const char* CATALOGUE_BOTTOM_ACTIONS[] = {
        "Refresh", "Back"
    };
}

AddBooksPage::AddBooksPage(ReaderPage& nextReaderPage)
    : readerPage(nextReaderPage)
{
}

void AddBooksPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;
    state = State::Fetching;
    completedBookId = "";
    completedBookTitle = "";

    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("ADD BOOKS", batteryPercent);
        drawMessage("Fetching book catalogue...");
        drawFooter();
    }
    while (display.nextPage());

    fetchAndRenderResult();
}

bool AddBooksPage::handleInput(const InputState& input)
{
    BookSync& sync = getBookSync();
    if (state == State::DownloadComplete)
    {
        const uint8_t previousIndex = selectedCompleteOption;
        if (!moveSelection(
            input,
            selectedCompleteOption,
            DOWNLOAD_COMPLETE_OPTION_COUNT
        ))
        {
            return input.upPressed || input.downPressed;
        }
        redrawMessageSelection(
            "Download complete",
            completedBookTitle.c_str(),
            DOWNLOAD_COMPLETE_OPTIONS,
            DOWNLOAD_COMPLETE_OPTION_COUNT,
            previousIndex,
            selectedCompleteOption
        );
        return true;
    }

    const uint8_t availableBookCount = getAvailableBookCount();
    if (
        state == State::Catalogue &&
        availableBookCount > 0
    ) {
        const uint8_t catalogueItemCount = availableBookCount;
        const uint8_t selectableItemCount = catalogueItemCount + 2;
        const SelectListState previousState = listState;
        if (input.upPressed && !input.downPressed)
        {
            moveSelectListUp(
                listState,
                selectableItemCount
            );
        }
        else if (input.downPressed && !input.upPressed)
        {
            moveSelectListDown(
                listState,
                selectableItemCount
            );
        }
        else
        {
            return false;
        }

        const char* titles[MAX_CATALOG_ITEMS];
        getBookTitles(titles);
        redrawSelectListAfterMove(
            titles,
            catalogueItemCount,
            previousState,
            listState,
            TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTIONS,
            CATALOGUE_BOTTOM_ACTIONS
        );
        return true;
    }

    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    const uint8_t actionCount = getActions(actions, labels);
    const uint8_t previousIndex = selectedActionIndex;
    if (!moveSelection(input, selectedActionIndex, actionCount))
    {
        return input.upPressed || input.downPressed;
    }
    String details;
    if (state == State::Catalogue)
    {
        details = availableBookCount;
        details += availableBookCount == 1
            ? " book available"
            : " books available";
    }
    else if (syncResult == BookSyncResult::HttpError)
    {
        details = "HTTP ";
        details += sync.getHttpStatus();
    }
    redrawMessageSelection(
        state == State::Catalogue
            ? "All Books Downloaded"
            : getBookSyncResultText(syncResult),
        details.length() > 0 ? details.c_str() : nullptr,
        labels,
        actionCount,
        previousIndex,
        selectedActionIndex
    );
    return true;
}

NavigationRequest AddBooksPage::select()
{
    if (state == State::DownloadComplete)
    {
        if (selectedCompleteOption == 0)
        {
            const CachedBook* book = findCachedBook(completedBookId.c_str());
            if (book == nullptr || !readerPage.open(book, getCachedBookPage(*book)))
            {
                downloadStatus = "Could not open book";
                state = State::Catalogue;
                drawResultContent();
                return noNavigation();
            }
            return navigateTo(PageId::ContinueReading);
        }
        if (selectedCompleteOption == 1)
        {
            state = State::Catalogue;
            drawResultContent();
            return noNavigation();
        }
        return navigateHome();
    }

    if (
        state == State::Catalogue &&
        getAvailableBookCount() > 0
    ) {
        const uint8_t bookCount = getAvailableBookCount();
        if (listState.selectedIndex == bookCount)
        {
            refresh();
            return noNavigation();
        }
        if (listState.selectedIndex == bookCount + 1)
        {
            return navigateBack();
        }
        downloadSelectedBook();
        return noNavigation();
    }

    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    const uint8_t actionCount = getActions(actions, labels);

    if (actionCount == 0)
    {
        return noNavigation();
    }

    if (selectedActionIndex >= actionCount) selectedActionIndex = 0;
    switch (actions[selectedActionIndex])
    {
        case Action::Retry:
            refresh();
            return noNavigation();
        case Action::WifiSettings:
            return ADD_BOOKS_OFFLINE_OPTIONS[0];
        case Action::Back:
            return navigateBack();
    }

    return noNavigation();
}

uint8_t AddBooksPage::getActions(
    Action* actions,
    const char** labels
) const {
    uint8_t count = 0;
    if (syncResult == BookSyncResult::NotConnected)
    {
        actions[count] = Action::WifiSettings;
        labels[count++] = getPageTitle(PageId::WiFiSettings);
    }
    else
    {
        actions[count] = Action::Retry;
        labels[count++] = "Refresh";
    }
    actions[count] = Action::Back;
    labels[count++] = "Back";
    return count;
}

void AddBooksPage::refresh()
{
    state = State::Fetching;
    drawLoadingContent();
    fetchAndRenderResult();
}

void AddBooksPage::fetchAndRenderResult()
{
    downloadStatus = "";
    selectedActionIndex = 0;
    syncResult = getBookSync().fetchManifest();
    Serial.print(F("[BookSync] Result: "));
    Serial.print(static_cast<uint8_t>(syncResult));
    Serial.print(F(", remote books: "));
    Serial.println(getBookSync().getBookCount());
    if (syncResult == BookSyncResult::Success)
    {
        state = State::Catalogue;
        resetSelectList(listState);
    }
    else
    {
        state = State::Error;
    }
    drawResultContent();
}

void AddBooksPage::drawLoadingContent()
{
    setPageContentPartialWindow();
    display.firstPage();
    do
    {
        clearPageContent();
        drawMessage("Fetching book catalogue...");
    }
    while (display.nextPage());
}

void AddBooksPage::drawResultContent()
{
    BookSync& sync = getBookSync();
    const uint8_t availableBookCount = getAvailableBookCount();
    String details;
    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    const uint8_t actionCount = getActions(actions, labels);

    if (state == State::Catalogue)
    {
        details = availableBookCount;
        details += availableBookCount == 1
            ? " book available"
            : " books available";
    }
    else if (syncResult == BookSyncResult::HttpError)
    {
        details = "HTTP ";
        details += sync.getHttpStatus();
    }
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("ADD BOOKS", batteryPercent);
        if (
            state == State::Catalogue &&
            availableBookCount > 0
        ) {
            const char* titles[MAX_CATALOG_ITEMS];
            getBookTitles(titles);
            drawSelectList(
                titles,
                availableBookCount,
                listState,
                TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTIONS,
                CATALOGUE_BOTTOM_ACTIONS
            );
        }
        else
        {
            drawMessage(
                state == State::Catalogue
                    ? "All Books Downloaded"
                    : getBookSyncResultText(syncResult),
                details.length() > 0 ? details.c_str() : nullptr,
                labels,
                actionCount,
                selectedActionIndex
            );
        }
        drawFooter(
            downloadStatus.length() > 0
                ? downloadStatus.c_str()
                : nullptr
        );
    }
    while (display.nextPage());
}

void AddBooksPage::drawDownloadCompleteContent()
{
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("ADD BOOKS", batteryPercent);
        drawMessage(
            "Download complete",
            completedBookTitle.c_str(),
            DOWNLOAD_COMPLETE_OPTIONS,
            DOWNLOAD_COMPLETE_OPTION_COUNT,
            selectedCompleteOption
        );
        drawFooter();
    }
    while (display.nextPage());
}

void AddBooksPage::getBookTitles(const char** titles) const
{
    const uint8_t availableBookCount = getAvailableBookCount();
    for (uint8_t index = 0; index < availableBookCount; index++)
    {
        const RemoteBook* book = getAvailableBook(index);
        titles[index] = book == nullptr ? "" : book->title;
    }
}

uint8_t AddBooksPage::getAvailableBookCount() const
{
    BookSync& sync = getBookSync();
    uint8_t availableCount = 0;
    for (uint8_t index = 0; index < sync.getBookCount(); index++)
    {
        if (findCachedBook(sync.getBook(index).id) == nullptr)
        {
            availableCount++;
        }
    }
    return availableCount;
}

const RemoteBook* AddBooksPage::getAvailableBook(
    uint8_t availableIndex
) const {
    BookSync& sync = getBookSync();
    uint8_t currentAvailableIndex = 0;
    for (uint8_t index = 0; index < sync.getBookCount(); index++)
    {
        const RemoteBook& book = sync.getBook(index);
        if (findCachedBook(book.id) != nullptr) continue;
        if (currentAvailableIndex == availableIndex) return &book;
        currentAvailableIndex++;
    }
    return nullptr;
}

void AddBooksPage::downloadSelectedBook()
{
    BookSync& sync = getBookSync();
    const RemoteBook* selectedBook = getAvailableBook(listState.selectedIndex);
    if (selectedBook == nullptr) return;
    const RemoteBook& book = *selectedBook;
    state = State::Downloading;

    setPageContentPartialWindow();
    display.firstPage();
    do
    {
        clearPageContent();
        drawMessage("Downloading", book.title);
    }
    while (display.nextPage());

    const BookDownloadResult result = sync.downloadBook(book);
    Serial.print(F("[BookSync] Download result for '"));
    Serial.print(book.title);
    Serial.print(F("': "));
    Serial.println(static_cast<uint8_t>(result));

    if (result == BookDownloadResult::Success)
    {
        const BookInstallResult installResult = installCachedBook(
            book.id,
            book.title,
            sync.getTemporaryDownloadPath()
        );
        Serial.print(F("[BookSync] Install result for '"));
        Serial.print(book.title);
        Serial.print(F("': "));
        Serial.println(static_cast<uint8_t>(installResult));
        downloadStatus = getBookInstallResultText(installResult);
        if (installResult == BookInstallResult::Success)
        {
            completedBookId = book.id;
            completedBookTitle = book.title;
            selectedCompleteOption = 0;
            state = State::DownloadComplete;
            drawDownloadCompleteContent();
            return;
        }
    }
    else
    {
        downloadStatus = getBookDownloadResultText(result);
    }
    state = State::Catalogue;
    drawResultContent();
}

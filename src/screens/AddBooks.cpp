#include "screens/AddBooks.h"

#include <stdio.h>

#include "Display.h"
#include "DisplayRefresh.h"
#include "Theme.h"
#include "books/BookSync.h"
#include "books/BookCache.h"
#include "books/BookServerSettings.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "components/SelectList.h"
#include "components/Selection.h"
#include "helpers/StorageText.h"
#include "navigation/PageRegistry.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr const char* DOWNLOAD_COMPLETE_OPTIONS[] = {
        "Read", "Back to Catalogue", "Main Menu"
    };
    constexpr const char* CATALOGUE_BOTTOM_ACTIONS[] = {
        "Refresh", "Configure", "Back"
    };
}

AddBooksPage::AddBooksPage(ReaderPage& nextReaderPage)
    : readerPage(nextReaderPage)
{
}

void AddBooksPage::onEnter()
{
    completedBookId = "";
    completedBookTitle = "";
    downloadStatus = "";

    const uint32_t serverRevision = getBookServerSettings().getRevision();
    BookSync& sync = getBookSync();
    if (sync.hasManifestResultFor(serverRevision))
    {
        catalogueFetchAttempted = true;
        catalogueServerRevision = serverRevision;
        syncResult = sync.getLastManifestResult();
    }
    if (
        catalogueFetchAttempted &&
        catalogueServerRevision != serverRevision
    ) {
        catalogueFetchAttempted = false;
    }
    if (
        catalogueFetchAttempted &&
        syncResult == BookSyncResult::NotConnected &&
        getWifiManager().isConnected()
    ) {
        catalogueFetchAttempted = false;
    }

    if (!catalogueFetchAttempted)
    {
        state = State::Fetching;
        fetchPending = true;
        return;
    }

    fetchPending = false;
    state = syncResult == BookSyncResult::Success
        ? State::Catalogue
        : State::Error;
}

void AddBooksPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;

    setPageRefreshWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("ADD BOOKS", batteryPercent);
        drawCurrentContent();
        drawStorageFooter();
    }
    while (display.nextPage());

    if (fetchPending)
    {
        fetchPending = false;
        fetchAndRenderResult();
    }
}

bool AddBooksPage::handleInput(const InputState& input)
{
    BookSync& sync = getBookSync();
    if (state == State::DownloadFailed)
    {
        FailureAction actions[MAX_FAILURE_OPTION_COUNT];
        const char* labels[MAX_FAILURE_OPTION_COUNT];
        const uint8_t optionCount = getFailureOptions(actions, labels);
        const uint8_t previousIndex = selectedFailureOption;
        if (!moveSelection(
            input,
            selectedFailureOption,
            optionCount
        )) {
            return input.upPressed || input.downPressed;
        }
        redrawMessageSelection(
            downloadFailureMessage.c_str(),
            completedBookTitle.c_str(),
            labels,
            optionCount,
            previousIndex,
            selectedFailureOption
        );
        return true;
    }

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
        const uint8_t selectableItemCount = catalogueItemCount + 3;
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
            TWO_LINE_SELECT_LIST_WITH_THREE_BOTTOM_ACTIONS,
            CATALOGUE_BOTTOM_ACTIONS
        );
        redrawStorageFooter();
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
    if (state == State::DownloadFailed)
    {
        FailureAction actions[MAX_FAILURE_OPTION_COUNT];
        const char* labels[MAX_FAILURE_OPTION_COUNT];
        const uint8_t optionCount = getFailureOptions(actions, labels);
        if (selectedFailureOption >= optionCount) selectedFailureOption = 0;
        switch (actions[selectedFailureOption])
        {
            case FailureAction::Retry:
                state = State::Catalogue;
                for (
                    uint8_t index = 0;
                    index < getAvailableBookCount();
                    index++
                ) {
                    const RemoteBook* book = getAvailableBook(index);
                    if (
                        book != nullptr &&
                        completedBookId.equals(book->id)
                    ) {
                        listState.selectedIndex = index;
                        downloadSelectedBook();
                        return noNavigation();
                    }
                }
                drawBody();
                return noNavigation();
            case FailureAction::Settings:
                downloadStatus = "";
                state = State::Catalogue;
                return navigateTo(PageId::Settings);
            case FailureAction::MyBooks:
                downloadStatus = "";
                state = State::Catalogue;
                return navigateTo(PageId::MyBooks);
            case FailureAction::Back:
                downloadStatus = "";
                state = State::Catalogue;
                drawBody();
                return noNavigation();
        }
        return noNavigation();
    }

    if (state == State::DownloadComplete)
    {
        if (selectedCompleteOption == 0)
        {
            const CachedBook* book = findCachedBook(completedBookId.c_str());
            if (book == nullptr || !readerPage.open(book, getCachedBookPage(*book)))
            {
                downloadStatus = "Could not open book";
                state = State::Catalogue;
                drawBody();
                return noNavigation();
            }
            return navigateTo(PageId::ContinueReading);
        }
        if (selectedCompleteOption == 1)
        {
            downloadStatus = "";
            state = State::Catalogue;
            drawBody();
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
            return navigateTo(PageId::Settings);
        }
        if (listState.selectedIndex == bookCount + 2)
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
        case Action::Configure:
            return navigateTo(PageId::Settings);
        case Action::Back:
            return navigateBack();
    }

    return noNavigation();
}

bool AddBooksPage::handleConnectivityStateChange(
    uint8_t nextBatteryPercent,
    bool wifiStateChanged,
    bool portalStateChanged
) {
    (void) portalStateChanged;
    batteryPercent = nextBatteryPercent;
    if (
        wifiStateChanged &&
        state == State::Error &&
        syncResult == BookSyncResult::NotConnected &&
        getWifiManager().isConnected()
    ) {
        refresh();
    }

    return false;
}

uint8_t AddBooksPage::getActions(
    Action* actions,
    const char** labels
) const {
    uint8_t count = 0;
    if (syncResult == BookSyncResult::NotConfigured)
    {
        actions[count] = Action::Configure;
        labels[count++] = "Configure";
    }
    else if (syncResult == BookSyncResult::NotConnected)
    {
        actions[count] = Action::Configure;
        labels[count++] = "Configure";
    }
    else
    {
        actions[count] = Action::Retry;
        labels[count++] = "Refresh";
        actions[count] = Action::Configure;
        labels[count++] = "Configure";
    }
    actions[count] = Action::Back;
    labels[count++] = "Back";
    return count;
}

void AddBooksPage::refresh()
{
    state = State::Fetching;
    drawBody();
    fetchAndRenderResult();
}

void AddBooksPage::fetchAndRenderResult()
{
    catalogueFetchAttempted = true;
    catalogueServerRevision = getBookServerSettings().getRevision();
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
    drawBody();
}

void AddBooksPage::drawBody()
{
    setPageBodyPartialWindow();
    display.firstPage();
    do
    {
        clearPageBody();
        drawCurrentContent();
        drawStorageFooter();
    }
    while (display.nextPage());
}

void AddBooksPage::drawCurrentContent()
{
    if (state == State::Fetching)
    {
        drawMessage("Fetching book catalogue...");
        return;
    }
    if (state == State::Downloading)
    {
        drawMessage("Downloading", completedBookTitle.c_str());
        return;
    }
    if (state == State::DownloadComplete)
    {
        drawMessage(
            "Download complete",
            completedBookTitle.c_str(),
            DOWNLOAD_COMPLETE_OPTIONS,
            DOWNLOAD_COMPLETE_OPTION_COUNT,
            selectedCompleteOption
        );
        return;
    }
    if (state == State::DownloadFailed)
    {
        FailureAction failureActions[MAX_FAILURE_OPTION_COUNT];
        const char* failureLabels[MAX_FAILURE_OPTION_COUNT];
        const uint8_t failureOptionCount = getFailureOptions(
            failureActions,
            failureLabels
        );
        drawMessage(
            downloadFailureMessage.c_str(),
            completedBookTitle.c_str(),
            failureLabels,
            failureOptionCount,
            selectedFailureOption
        );
        return;
    }

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
    if (state == State::Catalogue && availableBookCount > 0)
    {
        const char* titles[MAX_CATALOG_ITEMS];
        getBookTitles(titles);
        drawSelectList(
            titles,
            availableBookCount,
            listState,
            TWO_LINE_SELECT_LIST_WITH_THREE_BOTTOM_ACTIONS,
            CATALOGUE_BOTTOM_ACTIONS
        );
        return;
    }

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

void AddBooksPage::showDownloadFailure(
    const char* message,
    const char* bookId,
    const char* bookTitle,
    FailureAction primaryAction
) {
    downloadFailureMessage = message;
    completedBookId = bookId;
    completedBookTitle = bookTitle;
    failurePrimaryAction = primaryAction;
    selectedFailureOption = 0;
    state = State::DownloadFailed;
    drawBody();
}

uint8_t AddBooksPage::getFailureOptions(
    FailureAction* actions,
    const char** labels
) const {
    actions[0] = failurePrimaryAction;
    switch (failurePrimaryAction)
    {
        case FailureAction::Retry: labels[0] = "Retry"; break;
        case FailureAction::Settings: labels[0] = "Configure"; break;
        case FailureAction::MyBooks: labels[0] = "My Books"; break;
        case FailureAction::Back: labels[0] = "Back to Catalogue"; break;
    }
    actions[1] = FailureAction::Back;
    labels[1] = "Back to Catalogue";
    return MAX_FAILURE_OPTION_COUNT;
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

void AddBooksPage::getFooterText(
    char* leftText,
    size_t leftTextSize,
    char* rightText,
    size_t rightTextSize
) const {
    leftText[0] = '\0';
    const RemoteBook* selectedBook = nullptr;
    if (
        (state == State::Catalogue || state == State::Downloading) &&
        listState.selectedIndex < getAvailableBookCount()
    ) {
        selectedBook = getAvailableBook(listState.selectedIndex);
    }
    else if (
        state == State::DownloadComplete ||
        state == State::DownloadFailed
    ) {
        BookSync& sync = getBookSync();
        for (uint8_t index = 0; index < sync.getBookCount(); index++)
        {
            const RemoteBook& book = sync.getBook(index);
            if (completedBookId.equals(book.id))
            {
                selectedBook = &book;
                break;
            }
        }
    }

    if (selectedBook != nullptr)
    {
        if (selectedBook->sizeBytes == 0)
        {
            snprintf(leftText, leftTextSize, "Unknown");
        }
        else
        {
            char sizeText[16];
            formatStorageSize(
                selectedBook->sizeBytes,
                sizeText,
                sizeof(sizeText)
            );
            snprintf(leftText, leftTextSize, "%s", sizeText);
        }
    }

    char freeText[16];
    formatStorageSize(
        getAvailableBookStorageBytes(),
        freeText,
        sizeof(freeText)
    );
    snprintf(rightText, rightTextSize, "Free: %s", freeText);
}

void AddBooksPage::drawStorageFooter() const
{
    char leftText[32];
    char rightText[32];
    getFooterText(
        leftText, sizeof(leftText), rightText, sizeof(rightText)
    );
    drawFooter(leftText[0] == '\0' ? nullptr : leftText, rightText);
}

void AddBooksPage::redrawStorageFooter() const
{
    char leftText[32];
    char rightText[32];
    getFooterText(
        leftText, sizeof(leftText), rightText, sizeof(rightText)
    );
    redrawFooter(leftText[0] == '\0' ? nullptr : leftText, rightText);
}

void AddBooksPage::downloadSelectedBook()
{
    BookSync& sync = getBookSync();
    const RemoteBook* selectedBook = getAvailableBook(listState.selectedIndex);
    if (selectedBook == nullptr) return;
    const RemoteBook& book = *selectedBook;
    completedBookId = book.id;
    completedBookTitle = book.title;
    state = State::Downloading;
    drawBody();

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
            drawBody();
            return;
        }
        if (installResult == BookInstallResult::AlreadyCached)
        {
            state = State::Catalogue;
            drawBody();
            return;
        }
        const FailureAction action =
            installResult == BookInstallResult::LibraryFull ||
            installResult == BookInstallResult::StorageError
                ? FailureAction::MyBooks
                : FailureAction::Retry;
        showDownloadFailure(
            getBookInstallResultText(installResult),
            book.id,
            book.title,
            action
        );
        return;
    }

    downloadStatus = getBookDownloadResultText(result);
    FailureAction action = FailureAction::Retry;
    if (result == BookDownloadResult::NotConnected)
    {
        action = FailureAction::Settings;
    }
    else if (
        result == BookDownloadResult::NotEnoughSpace ||
        result == BookDownloadResult::StorageError
    ) {
        action = FailureAction::MyBooks;
    }
    showDownloadFailure(
        downloadStatus.c_str(), book.id, book.title, action
    );
}

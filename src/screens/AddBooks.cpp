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
#include "navigation/PageRegistry.h"

void AddBooksPage::draw(uint8_t nextBatteryPercent)
{
    batteryPercent = nextBatteryPercent;

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
    if (
        syncResult == BookSyncResult::Success &&
        sync.getBookCount() > 0
    ) {
        const SelectListState previousState = listState;
        if (input.upPressed && !input.downPressed)
        {
            moveSelectListUp(
                listState,
                sync.getBookCount()
            );
        }
        else if (input.downPressed && !input.upPressed)
        {
            moveSelectListDown(
                listState,
                sync.getBookCount()
            );
        }
        else
        {
            return false;
        }

        const char* titles[MAX_CATALOG_BOOKS];
        getBookTitles(titles);
        redrawSelectListAfterMove(
            titles,
            sync.getBookCount(),
            previousState,
            listState,
            TWO_LINE_SELECT_LIST
        );
        return true;
    }

    return input.upPressed || input.downPressed;
}

NavigationRequest AddBooksPage::select()
{
    if (
        syncResult == BookSyncResult::Success &&
        getBookSync().getBookCount() > 0
    ) {
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

    switch (actions[0])
    {
        case Action::Retry:
            refresh();
            return noNavigation();
        case Action::WifiSettings:
            return ADD_BOOKS_OFFLINE_OPTIONS[0];
    }

    return noNavigation();
}

uint8_t AddBooksPage::getActions(
    Action* actions,
    const char** labels
) const {
    if (syncResult == BookSyncResult::RequestFailed)
    {
        actions[0] = Action::Retry;
        labels[0] = "Retry";
        return 1;
    }

    if (syncResult == BookSyncResult::NotConnected)
    {
        actions[0] = Action::WifiSettings;
        labels[0] = getPageTitle(PageId::WiFiSettings);
        return 1;
    }

    return 0;
}

void AddBooksPage::refresh()
{
    drawLoadingContent();
    fetchAndRenderResult();
}

void AddBooksPage::fetchAndRenderResult()
{
    downloadStatus = "";
    syncResult = getBookSync().fetchManifest();
    Serial.print(F("[BookSync] Result: "));
    Serial.print(static_cast<uint8_t>(syncResult));
    Serial.print(F(", remote books: "));
    Serial.println(getBookSync().getBookCount());
    if (syncResult == BookSyncResult::Success)
    {
        resetSelectList(listState);
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
    String details;
    Action actions[MAX_ACTION_COUNT];
    const char* labels[MAX_ACTION_COUNT];
    const uint8_t actionCount = getActions(actions, labels);

    if (syncResult == BookSyncResult::Success)
    {
        details = sync.getBookCount();
        details += sync.getBookCount() == 1
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
            syncResult == BookSyncResult::Success &&
            sync.getBookCount() > 0
        ) {
            const char* titles[MAX_CATALOG_BOOKS];
            getBookTitles(titles);
            drawSelectList(
                titles,
                sync.getBookCount(),
                listState,
                TWO_LINE_SELECT_LIST
            );
        }
        else
        {
            drawMessage(
                syncResult == BookSyncResult::Success
                    ? "No books available"
                    : getBookSyncResultText(syncResult),
                details.length() > 0 ? details.c_str() : nullptr,
                labels,
                actionCount,
                0
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

void AddBooksPage::getBookTitles(const char** titles) const
{
    BookSync& sync = getBookSync();
    for (uint8_t index = 0; index < sync.getBookCount(); index++)
    {
        titles[index] = sync.getBook(index).title;
    }
}

void AddBooksPage::downloadSelectedBook()
{
    BookSync& sync = getBookSync();
    if (listState.selectedIndex >= sync.getBookCount()) return;
    const RemoteBook& book = sync.getBook(listState.selectedIndex);

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
    }
    else
    {
        downloadStatus = getBookDownloadResultText(result);
    }
    drawResultContent();
}

#include "screens/AddBooks.h"

#include "Display.h"
#include "Theme.h"
#include "books/BookSync.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "navigation/PageRegistry.h"

void AddBooksPage::draw(uint8_t batteryPercent)
{
    BookSync& sync = getBookSync();

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

    syncResult = sync.fetchManifest();
    String details;
    const char* options[MAX_NAVIGATION_OPTIONS];
    uint8_t optionCount = 0;

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
    else if (syncResult == BookSyncResult::NotConnected)
    {
        optionCount = ADD_BOOKS_OFFLINE_OPTION_COUNT;
        getNavigationOptionLabels(
            ADD_BOOKS_OFFLINE_OPTIONS,
            options,
            optionCount
        );
    }

    setPageContentPartialWindow();
    display.firstPage();
    do
    {
        clearPageContent();
        drawMessage(
            getBookSyncResultText(syncResult),
            details.length() > 0 ? details.c_str() : nullptr,
            options,
            optionCount,
            0
        );
    }
    while (display.nextPage());
}

bool AddBooksPage::handleInput(const InputState& input)
{
    return input.upPressed || input.downPressed;
}

NavigationRequest AddBooksPage::select()
{
    return syncResult == BookSyncResult::NotConnected
        ? ADD_BOOKS_OFFLINE_OPTIONS[0]
        : noNavigation();
}

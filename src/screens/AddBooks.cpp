#include "screens/AddBooks.h"

#include "Display.h"
#include "Theme.h"
#include "books/BookSync.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
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
    return input.upPressed || input.downPressed;
}

NavigationRequest AddBooksPage::select()
{
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
    syncResult = getBookSync().fetchManifest();
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
    setPageContentPartialWindow();
    display.firstPage();
    do
    {
        clearPageContent();
        drawMessage(
            getBookSyncResultText(syncResult),
            details.length() > 0 ? details.c_str() : nullptr,
            labels,
            actionCount,
            0
        );
    }
    while (display.nextPage());
}

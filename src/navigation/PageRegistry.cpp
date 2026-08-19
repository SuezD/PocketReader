#include "navigation/PageRegistry.h"

#include "screens/AddBooks.h"
#include "screens/BookPages.h"
#include "screens/MainMenu.h"
#include "screens/MyBooks.h"
#include "screens/Reader.h"
#include "screens/WifiSettings.h"
#include "screens/WifiNetworkPages.h"
#include "wifi/SelectedWifiNetwork.h"
#include "books/SelectedCachedBook.h"

namespace
{
    MainMenuPage mainMenuPage;
    ReaderPage continueReadingPage;
    SelectedCachedBook selectedCachedBook;
    MyBooksPage myBooksPage(selectedCachedBook);
    BookActionsPage bookActionsPage(selectedCachedBook, continueReadingPage);
    DeleteBookPage deleteBookPage(selectedCachedBook, continueReadingPage);
    AddBooksPage addBooksPage(continueReadingPage);
    SelectedWifiNetwork selectedWifiNetwork;
    WifiSettingsPage wiFiSettingsPage(selectedWifiNetwork);
    WifiNetworkActionsPage wifiNetworkActionsPage(selectedWifiNetwork);
    WifiForgetNetworkPage wifiForgetNetworkPage(selectedWifiNetwork);
    WifiSetupPage wifiSetupPage;

    const PageDefinition PAGES[] = {
#define PAGE(id, title, instance) { PageId::id, title, &instance },
#include "navigation/PageRegistry.def"
#undef PAGE
    };
}

const NavigationRequest MAIN_MENU_OPTIONS[] = {
    navigateTo(PageId::ContinueReading),
    navigateTo(PageId::MyBooks),
    navigateTo(PageId::AddBooks),
    navigateTo(PageId::WiFiSettings)
};

const uint8_t MAIN_MENU_OPTION_COUNT =
    sizeof(MAIN_MENU_OPTIONS) / sizeof(MAIN_MENU_OPTIONS[0]);

const NavigationRequest READER_EMPTY_OPTIONS[] = {
    navigateHome(),
    replaceWith(PageId::MyBooks),
    replaceWith(PageId::AddBooks)
};

const uint8_t READER_EMPTY_OPTION_COUNT =
    sizeof(READER_EMPTY_OPTIONS) / sizeof(READER_EMPTY_OPTIONS[0]);

const NavigationRequest MY_BOOKS_EMPTY_OPTIONS[] = {
    navigateHome(),
    navigateTo(PageId::AddBooks)
};

const uint8_t MY_BOOKS_EMPTY_OPTION_COUNT =
    sizeof(MY_BOOKS_EMPTY_OPTIONS) / sizeof(MY_BOOKS_EMPTY_OPTIONS[0]);

const NavigationRequest ADD_BOOKS_OFFLINE_OPTIONS[] = {
    navigateTo(PageId::WiFiSettings)
};

const uint8_t ADD_BOOKS_OFFLINE_OPTION_COUNT =
    sizeof(ADD_BOOKS_OFFLINE_OPTIONS) /
    sizeof(ADD_BOOKS_OFFLINE_OPTIONS[0]);

const PageDefinition* findPage(PageId id)
{
    for (const PageDefinition& page : PAGES)
    {
        if (page.id == id)
        {
            return &page;
        }
    }

    return nullptr;
}

void initializeRegisteredPages()
{
    for (const PageDefinition& page : PAGES)
    {
        if (page.page != nullptr)
        {
            page.page->onStartup();
        }
    }
}

const char* getPageTitle(PageId id)
{
    const PageDefinition* page = findPage(id);
    return page == nullptr ? "Unknown Page" : page->title;
}

void getNavigationOptionLabels(
    const NavigationRequest* options,
    const char** labels,
    uint8_t count
) {
    for (uint8_t index = 0; index < count; index++)
    {
        labels[index] = getPageTitle(
            options[index].destination
        );
    }
}

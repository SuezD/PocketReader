#include "navigation/PageRegistry.h"

#include "screens/AddBooks.h"
#include "screens/MainMenu.h"
#include "screens/MyBooks.h"
#include "screens/Reader.h"
#include "screens/WifiSettings.h"
#include "screens/WifiNetworkPages.h"
#include "wifi/SelectedWifiNetwork.h"

namespace
{
    MainMenuPage mainMenuPage;
    ReaderPage continueReadingPage;
    MyBooksPage myBooksPage(continueReadingPage);
    AddBooksPage addBooksPage;
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
    { NavigationMode::Push, PageId::ContinueReading },
    { NavigationMode::Push, PageId::MyBooks },
    { NavigationMode::Push, PageId::AddBooks },
    { NavigationMode::Push, PageId::WiFiSettings }
};

const uint8_t MAIN_MENU_OPTION_COUNT =
    sizeof(MAIN_MENU_OPTIONS) / sizeof(MAIN_MENU_OPTIONS[0]);

const NavigationRequest READER_EMPTY_OPTIONS[] = {
    { NavigationMode::Home, PageId::MainMenu },
    { NavigationMode::Replace, PageId::MyBooks },
    { NavigationMode::Replace, PageId::AddBooks }
};

const uint8_t READER_EMPTY_OPTION_COUNT =
    sizeof(READER_EMPTY_OPTIONS) / sizeof(READER_EMPTY_OPTIONS[0]);

const NavigationRequest MY_BOOKS_EMPTY_OPTIONS[] = {
    { NavigationMode::Push, PageId::MainMenu },
    { NavigationMode::Push, PageId::AddBooks }
};

const uint8_t MY_BOOKS_EMPTY_OPTION_COUNT =
    sizeof(MY_BOOKS_EMPTY_OPTIONS) / sizeof(MY_BOOKS_EMPTY_OPTIONS[0]);

const NavigationRequest ADD_BOOKS_OFFLINE_OPTIONS[] = {
    { NavigationMode::Push, PageId::WiFiSettings }
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

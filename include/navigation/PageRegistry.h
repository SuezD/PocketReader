#pragma once

#include "navigation/Navigation.h"
#include "navigation/Page.h"

struct PageDefinition
{
    PageId id;
    const char* title;
    Page* page;
};

const PageDefinition* findPage(PageId id);
void initializeRegisteredPages();
const char* getPageTitle(PageId id);
void getNavigationOptionLabels(
    const NavigationRequest* options,
    const char** labels,
    uint8_t count
);

extern const NavigationRequest MAIN_MENU_OPTIONS[];
extern const uint8_t MAIN_MENU_OPTION_COUNT;

extern const NavigationRequest READER_EMPTY_OPTIONS[];
extern const uint8_t READER_EMPTY_OPTION_COUNT;

extern const NavigationRequest MY_BOOKS_EMPTY_OPTIONS[];
extern const uint8_t MY_BOOKS_EMPTY_OPTION_COUNT;

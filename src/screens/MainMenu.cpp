#include "MainMenu.h"

#include "Display.h"
#include "Theme.h"

#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"

namespace
{
    const char* const ITEMS[] = {
        "Continue reading",
        "My Books",
        "Add Books",
        "Wi-Fi Settings"
    };

    constexpr uint8_t ITEM_COUNT =
        sizeof(ITEMS) / sizeof(ITEMS[0]);

    SelectListState listState;
}

void drawMainMenu(uint8_t batteryPercent)
{
    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);

        drawHeader("MAIN MENU", batteryPercent);

        drawSelectList(
            ITEMS,
            ITEM_COUNT,
            listState
        );

        drawFooter("Up / Down", "Select");
    }
    while (display.nextPage());
}

void redrawMainMenuList()
{
    const int contentTop = Theme::HEADER_HEIGHT + 1;
    const int contentBottom =
        display.height() - Theme::FOOTER_HEIGHT;

    display.setPartialWindow(
        0,
        contentTop,
        display.width(),
        contentBottom - contentTop
    );

    display.firstPage();

    do
    {
        display.fillRect(
            0,
            contentTop,
            display.width(),
            contentBottom - contentTop,
            Theme::BACKGROUND_COLOR
        );

        drawSelectList(
            ITEMS,
            ITEM_COUNT,
            listState
        );
    }
    while (display.nextPage());
}

void moveMainMenuUp()
{
    moveSelectListUp(listState, ITEM_COUNT);
}

void moveMainMenuDown()
{
    moveSelectListDown(listState, ITEM_COUNT);
}

MainMenuItem getSelectedMainMenuItem()
{
    return static_cast<MainMenuItem>(
        listState.selectedIndex
    );
}
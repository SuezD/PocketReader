#include "screens/MainMenu.h"

#include "Display.h"
#include "DisplayRefresh.h"
#include "Theme.h"
#include "navigation/PageRegistry.h"

#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"

void MainMenuPage::draw(uint8_t batteryPercent)
{
    const char* labels[MAX_NAVIGATION_OPTIONS];
    getNavigationOptionLabels(
        MAIN_MENU_OPTIONS,
        labels,
        MAIN_MENU_OPTION_COUNT
    );
    setPageRefreshWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("MAIN MENU", batteryPercent);
        drawSelectList(labels, MAIN_MENU_OPTION_COUNT, listState);
        drawFooter("Up / Down", "Select");
    }
    while (display.nextPage());
}

bool MainMenuPage::handleInput(const InputState& input)
{
    const SelectListState previousState = listState;

    if (input.upPressed && !input.downPressed)
    {
        moveSelectListUp(listState, MAIN_MENU_OPTION_COUNT);
    }
    else if (input.downPressed && !input.upPressed)
    {
        moveSelectListDown(listState, MAIN_MENU_OPTION_COUNT);
    }
    else
    {
        return false;
    }

    const char* labels[MAX_NAVIGATION_OPTIONS];
    getNavigationOptionLabels(
        MAIN_MENU_OPTIONS,
        labels,
        MAIN_MENU_OPTION_COUNT
    );
    redrawSelectListAfterMove(
        labels,
        MAIN_MENU_OPTION_COUNT,
        previousState,
        listState
    );

    return true;
}

NavigationRequest MainMenuPage::select()
{
    return MAIN_MENU_OPTIONS[listState.selectedIndex];
}

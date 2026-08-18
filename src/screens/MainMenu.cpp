#include "screens/MainMenu.h"

#include "Display.h"
#include "Theme.h"
#include "navigation/PageRegistry.h"

#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"

void MainMenuPage::redrawSelection(uint8_t previousIndex)
{
    const uint8_t selectedIndex = listState.selectedIndex;
    const uint8_t firstChangedIndex =
        min(previousIndex, selectedIndex);
    const uint8_t lastChangedIndex =
        max(previousIndex, selectedIndex);

    const int itemStride =
        Theme::SELECT_LIST_ITEM_HEIGHT +
        Theme::SELECT_LIST_ITEM_GAP;

    const int contentTop =
        Theme::HEADER_HEIGHT +
        1 +
        Theme::SELECT_LIST_VERTICAL_PADDING +
        firstChangedIndex * itemStride;

    const int contentHeight =
        (lastChangedIndex - firstChangedIndex) * itemStride +
        Theme::SELECT_LIST_ITEM_HEIGHT;

    display.setPartialWindow(
        Theme::PAGE_MARGIN,
        contentTop,
        Theme::SELECT_LIST_MARKER_WIDTH,
        contentHeight
    );

    display.firstPage();

    do
    {
        display.fillRect(
            Theme::PAGE_MARGIN,
            contentTop,
            Theme::SELECT_LIST_MARKER_WIDTH,
            contentHeight,
            Theme::BACKGROUND_COLOR
        );

        const int selectedRowY =
            Theme::HEADER_HEIGHT +
            1 +
            Theme::SELECT_LIST_VERTICAL_PADDING +
            selectedIndex * itemStride;

        display.fillRect(
            Theme::PAGE_MARGIN,
            selectedRowY,
            Theme::SELECT_LIST_MARKER_WIDTH,
            Theme::SELECT_LIST_ITEM_HEIGHT,
            Theme::TEXT_COLOR
        );
    }
    while (display.nextPage());
}

void MainMenuPage::draw(uint8_t batteryPercent)
{
    const char* labels[MAX_NAVIGATION_OPTIONS];
    getNavigationOptionLabels(
        MAIN_MENU_OPTIONS,
        labels,
        MAIN_MENU_OPTION_COUNT
    );
    display.setFullWindow();
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
    const uint8_t previousIndex = listState.selectedIndex;

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

    if (listState.selectedIndex != previousIndex)
    {
        redrawSelection(previousIndex);
    }

    return true;
}

NavigationRequest MainMenuPage::select()
{
    return MAIN_MENU_OPTIONS[listState.selectedIndex];
}

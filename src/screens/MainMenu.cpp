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

void redrawMainMenuSelection(MainMenuItem previousItem)
{
    const uint8_t previousIndex =
        static_cast<uint8_t>(previousItem);
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

bool moveMainMenuUp()
{
    const uint8_t previousIndex = listState.selectedIndex;
    moveSelectListUp(listState, ITEM_COUNT);
    return listState.selectedIndex != previousIndex;
}

bool moveMainMenuDown()
{
    const uint8_t previousIndex = listState.selectedIndex;
    moveSelectListDown(listState, ITEM_COUNT);
    return listState.selectedIndex != previousIndex;
}

MainMenuItem getSelectedMainMenuItem()
{
    return static_cast<MainMenuItem>(
        listState.selectedIndex
    );
}

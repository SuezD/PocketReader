#include "screens/MyBooks.h"

#include "Display.h"
#include "Theme.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/SelectList.h"

namespace
{
    SelectListState listState;

    void drawSelectionMarker(uint8_t selectedIndex)
    {
        const int itemStride =
            Theme::SELECT_LIST_ITEM_HEIGHT +
            Theme::SELECT_LIST_ITEM_GAP;

        const int rowY =
            Theme::HEADER_HEIGHT +
            1 +
            Theme::SELECT_LIST_VERTICAL_PADDING +
            selectedIndex * itemStride;

        display.fillRect(
            Theme::PAGE_MARGIN,
            rowY,
            Theme::SELECT_LIST_MARKER_WIDTH,
            Theme::SELECT_LIST_ITEM_HEIGHT,
            Theme::TEXT_COLOR
        );
    }
}

void drawMyBooks(uint8_t batteryPercent)
{
    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader("MY BOOKS", batteryPercent);

        drawSelectList(
            getCachedBookTitles(),
            getCachedBookCount(),
            listState
        );

        drawFooter();
    }
    while (display.nextPage());
}

void redrawMyBooksSelection(uint8_t previousIndex)
{
    const uint8_t selectedIndex = listState.selectedIndex;
    const uint8_t firstChangedIndex =
        min(previousIndex, selectedIndex);
    const uint8_t lastChangedIndex =
        max(previousIndex, selectedIndex);

    const int itemStride =
        Theme::SELECT_LIST_ITEM_HEIGHT +
        Theme::SELECT_LIST_ITEM_GAP;

    const int updateTop =
        Theme::HEADER_HEIGHT +
        1 +
        Theme::SELECT_LIST_VERTICAL_PADDING +
        firstChangedIndex * itemStride;

    const int updateHeight =
        (lastChangedIndex - firstChangedIndex) * itemStride +
        Theme::SELECT_LIST_ITEM_HEIGHT;

    display.setPartialWindow(
        Theme::PAGE_MARGIN,
        updateTop,
        Theme::SELECT_LIST_MARKER_WIDTH,
        updateHeight
    );

    display.firstPage();

    do
    {
        display.fillRect(
            Theme::PAGE_MARGIN,
            updateTop,
            Theme::SELECT_LIST_MARKER_WIDTH,
            updateHeight,
            Theme::BACKGROUND_COLOR
        );

        drawSelectionMarker(selectedIndex);
    }
    while (display.nextPage());
}

bool moveMyBooksUp()
{
    const uint8_t previousIndex = listState.selectedIndex;
    moveSelectListUp(listState, getCachedBookCount());
    return listState.selectedIndex != previousIndex;
}

bool moveMyBooksDown()
{
    const uint8_t previousIndex = listState.selectedIndex;
    moveSelectListDown(listState, getCachedBookCount());
    return listState.selectedIndex != previousIndex;
}

const CachedBook& getSelectedMyBook()
{
    return getCachedBook(listState.selectedIndex);
}

uint8_t getSelectedMyBookIndex()
{
    return listState.selectedIndex;
}

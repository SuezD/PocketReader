#include "SelectList.h"

#include "Display.h"
#include "Theme.h"
#include "helpers/TextUtils.h"

namespace
{
    constexpr size_t TEXT_BUFFER_SIZE = 64;

    uint8_t getVisibleItemCount()
    {
        const int contentTop =
            Theme::HEADER_HEIGHT +
            1 +
            Theme::SELECT_LIST_VERTICAL_PADDING;

        const int contentBottom =
            display.height() -
            Theme::FOOTER_HEIGHT -
            Theme::SELECT_LIST_VERTICAL_PADDING;

        const int contentHeight =
            contentBottom - contentTop;

        const int itemStride =
            Theme::SELECT_LIST_ITEM_HEIGHT +
            Theme::SELECT_LIST_ITEM_GAP;

        const uint8_t count =
            (contentHeight + Theme::SELECT_LIST_ITEM_GAP) /
            itemStride;

        return count > 0 ? count : 1;
    }

    void keepSelectionVisible(
        SelectListState& state,
        uint8_t itemCount
    ) {
        if (itemCount == 0)
        {
            resetSelectList(state);
            return;
        }

        if (state.selectedIndex >= itemCount)
        {
            state.selectedIndex = itemCount - 1;
        }

        const uint8_t visibleItemCount =
            getVisibleItemCount();

        if (state.selectedIndex < state.firstVisibleIndex)
        {
            state.firstVisibleIndex =
                state.selectedIndex;
        }
        else if (
            state.selectedIndex >=
            state.firstVisibleIndex + visibleItemCount
        ) {
            state.firstVisibleIndex =
                state.selectedIndex -
                visibleItemCount +
                1;
        }
    }
}

void resetSelectList(SelectListState& state)
{
    state.selectedIndex = 0;
    state.firstVisibleIndex = 0;
}

void moveSelectListUp(
    SelectListState& state,
    uint8_t itemCount
) {
    if (itemCount == 0 || state.selectedIndex == 0)
    {
        return;
    }

    state.selectedIndex--;
    keepSelectionVisible(state, itemCount);
}

void moveSelectListDown(
    SelectListState& state,
    uint8_t itemCount
) {
    if (
        itemCount == 0 ||
        state.selectedIndex >= itemCount - 1
    ) {
        return;
    }

    state.selectedIndex++;
    keepSelectionVisible(state, itemCount);
}

void drawSelectList(
    const char* const items[],
    uint8_t itemCount,
    SelectListState& state
) {
    if (items == nullptr || itemCount == 0)
    {
        return;
    }

    keepSelectionVisible(state, itemCount);

    const int contentLeft = Theme::PAGE_MARGIN;
    const int contentRight =
        display.width() - Theme::PAGE_MARGIN;

    const int rowWidth =
        contentRight - contentLeft;

    const uint16_t maximumTextWidth =
        rowWidth -
        (Theme::SELECT_LIST_HORIZONTAL_PADDING * 2) -
        Theme::SELECT_LIST_MARKER_WIDTH -
        Theme::SELECT_LIST_MARKER_GAP;

    const int listTop =
        Theme::HEADER_HEIGHT +
        1 +
        Theme::SELECT_LIST_VERTICAL_PADDING;

    const uint8_t visibleItemCount =
        getVisibleItemCount();

    const uint8_t finalVisibleIndex =
        min(
            static_cast<uint8_t>(
                state.firstVisibleIndex +
                visibleItemCount
            ),
            itemCount
        );

    display.setFont(Theme::BODY_FONT);

    for (
        uint8_t index = state.firstVisibleIndex;
        index < finalVisibleIndex;
        index++
    ) {
        const uint8_t visiblePosition =
            index - state.firstVisibleIndex;

        const int rowY =
            listTop +
            visiblePosition *
            (
                Theme::SELECT_LIST_ITEM_HEIGHT +
                Theme::SELECT_LIST_ITEM_GAP
            );

        const bool isSelected =
            index == state.selectedIndex;

        if (isSelected)
        {
            display.fillRect(
                contentLeft,
                rowY,
                Theme::SELECT_LIST_MARKER_WIDTH,
                Theme::SELECT_LIST_ITEM_HEIGHT,
                Theme::TEXT_COLOR
            );
        }

        char output[TEXT_BUFFER_SIZE];

        truncateToWidth(
            items[index],
            output,
            sizeof(output),
            maximumTextWidth,
            Theme::BODY_FONT
        );

        display.setTextColor(Theme::TEXT_COLOR);

        display.setCursor(
            contentLeft +
                Theme::SELECT_LIST_HORIZONTAL_PADDING +
                Theme::SELECT_LIST_MARKER_WIDTH +
                Theme::SELECT_LIST_MARKER_GAP,
            rowY +
                Theme::SELECT_LIST_TEXT_BASELINE
        );

        display.print(output);
    }

    display.setTextColor(Theme::TEXT_COLOR);
}

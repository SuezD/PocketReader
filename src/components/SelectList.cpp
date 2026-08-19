#include "SelectList.h"

#include <string.h>

#include "Display.h"
#include "Theme.h"
#include "helpers/TextUtils.h"
#include "Selection.h"

namespace
{
    constexpr size_t TEXT_BUFFER_SIZE = 64;

    int getListTop()
    {
        return Theme::HEADER_HEIGHT + 1 + Theme::SELECT_LIST_VERTICAL_PADDING;
    }

    int getListBottom(SelectListConfig config)
    {
        int bottom = display.height() - Theme::FOOTER_HEIGHT -
            Theme::SELECT_LIST_VERTICAL_PADDING;
        if (config.bottomActionCount > 0)
        {
            bottom -= config.bottomActionCount *
                (Theme::SELECT_LIST_ITEM_HEIGHT + Theme::SELECT_LIST_ITEM_GAP);
        }
        return bottom;
    }

    uint16_t getMaximumTextWidth()
    {
        const int rowWidth =
            display.width() - (Theme::PAGE_MARGIN * 2);
        return rowWidth -
            (Theme::SELECT_LIST_HORIZONTAL_PADDING * 2) -
            Theme::SELECT_LIST_MARKER_WIDTH -
            Theme::SELECT_LIST_MARKER_GAP;
    }

    bool needsSecondLine(
        const char* text,
        SelectListConfig config,
        uint16_t maximumTextWidth
    ) {
        return config.textLineCount > 1 &&
            getTextWidth(text, Theme::BODY_FONT) > maximumTextWidth;
    }

    int getItemHeight(
        const char* text,
        SelectListConfig config,
        uint16_t maximumTextWidth
    ) {
        return needsSecondLine(text, config, maximumTextWidth)
            ? Theme::SELECT_LIST_TWO_LINE_ITEM_HEIGHT
            : Theme::SELECT_LIST_ITEM_HEIGHT;
    }

    int getRowTop(
        const char* const items[],
        uint8_t index,
        const SelectListState& state,
        SelectListConfig config,
        uint16_t maximumTextWidth
    ) {
        int rowTop = getListTop();
        for (uint8_t current = state.firstVisibleIndex; current < index; current++)
        {
            rowTop += getItemHeight(
                items[current], config, maximumTextWidth
            ) + Theme::SELECT_LIST_ITEM_GAP;
        }
        return rowTop;
    }

    bool isVisible(
        const char* const items[],
        uint8_t index,
        const SelectListState& state,
        SelectListConfig config,
        uint16_t maximumTextWidth
    ) {
        if (index < state.firstVisibleIndex) return false;
        const int rowTop = getRowTop(
            items, index, state, config, maximumTextWidth
        );
        return rowTop + getItemHeight(
            items[index], config, maximumTextWidth
        ) <= getListBottom(config);
    }

    void keepSelectionVisible(
        const char* const items[],
        uint8_t itemCount,
        SelectListState& state,
        SelectListConfig config
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

        const uint16_t maximumTextWidth = getMaximumTextWidth();
        if (state.selectedIndex < state.firstVisibleIndex)
        {
            state.firstVisibleIndex = state.selectedIndex;
            return;
        }
        if (isVisible(
            items, state.selectedIndex, state, config, maximumTextWidth
        )) {
            return;
        }

        state.firstVisibleIndex = state.selectedIndex;
        int usedHeight = getItemHeight(
            items[state.selectedIndex], config, maximumTextWidth
        );
        const int availableHeight = getListBottom(config) - getListTop();

        while (state.firstVisibleIndex > 0)
        {
            const uint8_t previousIndex = state.firstVisibleIndex - 1;
            const int previousHeight = getItemHeight(
                items[previousIndex], config, maximumTextWidth
            );
            if (
                usedHeight + Theme::SELECT_LIST_ITEM_GAP + previousHeight >
                availableHeight
            ) {
                break;
            }
            usedHeight += Theme::SELECT_LIST_ITEM_GAP + previousHeight;
            state.firstVisibleIndex = previousIndex;
        }
    }

    size_t getFittingPrefixLength(
        const char* source,
        uint16_t maximumWidth
    ) {
        char candidate[TEXT_BUFFER_SIZE];
        size_t length = 0;
        while (source[length] != '\0' && length + 1 < sizeof(candidate))
        {
            candidate[length] = source[length];
            candidate[length + 1] = '\0';
            if (getTextWidth(candidate, Theme::BODY_FONT) > maximumWidth) break;
            length++;
        }
        return length;
    }

    void wrapTwoLineText(
        const char* source,
        char* firstLine,
        char* secondLine,
        uint16_t maximumWidth
    ) {
        firstLine[0] = '\0';
        secondLine[0] = '\0';
        if (source == nullptr || source[0] == '\0') return;

        const size_t fittingLength =
            getFittingPrefixLength(source, maximumWidth);
        size_t splitLength = fittingLength;
        for (size_t index = 0; index < fittingLength; index++)
        {
            if (source[index] == ' ') splitLength = index;
        }
        if (splitLength == 0) splitLength = fittingLength;

        const size_t firstLength = min(
            splitLength,
            static_cast<size_t>(TEXT_BUFFER_SIZE - 1)
        );
        memcpy(firstLine, source, firstLength);
        firstLine[firstLength] = '\0';

        const char* remainder = source + splitLength;
        while (remainder[0] == ' ') remainder++;
        truncateToWidth(
            remainder,
            secondLine,
            TEXT_BUFFER_SIZE,
            maximumWidth,
            Theme::BODY_FONT
        );
    }
}

void resetSelectList(SelectListState& state)
{
    state.selectedIndex = 0;
    state.firstVisibleIndex = 0;
}

void moveSelectListUp(SelectListState& state, uint8_t itemCount)
{
    selectPrevious(state.selectedIndex, itemCount);
}

void moveSelectListDown(SelectListState& state, uint8_t itemCount)
{
    selectNext(state.selectedIndex, itemCount);
}

void drawSelectList(
    const char* const items[],
    uint8_t itemCount,
    SelectListState& state,
    SelectListConfig config,
    const char* const bottomActions[]
) {
    if (items == nullptr || itemCount == 0) return;
    const bool bottomSelected =
        config.bottomActionCount > 0 && state.selectedIndex >= itemCount;
    if (bottomSelected)
    {
        SelectListState visibleState = state;
        visibleState.selectedIndex = itemCount - 1;
        keepSelectionVisible(items, itemCount, visibleState, config);
        state.firstVisibleIndex = visibleState.firstVisibleIndex;
    }
    else
    {
        keepSelectionVisible(items, itemCount, state, config);
    }

    const int contentLeft = Theme::PAGE_MARGIN;
    const int textX = contentLeft + Theme::SELECT_LIST_HORIZONTAL_PADDING +
        Theme::SELECT_LIST_MARKER_WIDTH + Theme::SELECT_LIST_MARKER_GAP;
    const uint16_t maximumTextWidth = getMaximumTextWidth();
    int rowTop = getListTop();
    display.setFont(Theme::BODY_FONT);
    display.setTextColor(Theme::TEXT_COLOR);

    for (uint8_t index = state.firstVisibleIndex; index < itemCount; index++)
    {
        const int itemHeight = getItemHeight(
            items[index], config, maximumTextWidth
        );
        if (rowTop + itemHeight > getListBottom(config)) break;

        if (index == state.selectedIndex)
        {
            display.fillRect(
                contentLeft,
                rowTop,
                Theme::SELECT_LIST_MARKER_WIDTH,
                itemHeight,
                Theme::TEXT_COLOR
            );
        }

        char firstLine[TEXT_BUFFER_SIZE];
        char secondLine[TEXT_BUFFER_SIZE];
        if (needsSecondLine(items[index], config, maximumTextWidth))
        {
            wrapTwoLineText(
                items[index], firstLine, secondLine, maximumTextWidth
            );
        }
        else
        {
            truncateToWidth(
                items[index], firstLine, sizeof(firstLine),
                maximumTextWidth, Theme::BODY_FONT
            );
            secondLine[0] = '\0';
        }

        display.setCursor(
            textX,
            rowTop + Theme::SELECT_LIST_TEXT_BASELINE
        );
        display.print(firstLine);
        if (secondLine[0] != '\0')
        {
            display.setCursor(
                textX,
                rowTop + Theme::SELECT_LIST_TEXT_BASELINE +
                    Theme::SELECT_LIST_TEXT_LINE_HEIGHT
            );
            display.print(secondLine);
        }
        rowTop += itemHeight + Theme::SELECT_LIST_ITEM_GAP;
    }

    if (config.bottomActionCount > 0 && bottomActions != nullptr)
    {
        int actionTop = getListBottom(config) + Theme::SELECT_LIST_ITEM_GAP;
        for (
            uint8_t actionIndex = 0;
            actionIndex < config.bottomActionCount;
            actionIndex++
        )
        {
            if (state.selectedIndex == itemCount + actionIndex)
            {
                display.fillRect(
                    contentLeft,
                    actionTop,
                    Theme::SELECT_LIST_MARKER_WIDTH,
                    Theme::SELECT_LIST_ITEM_HEIGHT,
                    Theme::TEXT_COLOR
                );
            }
            display.setCursor(
                textX,
                actionTop + Theme::SELECT_LIST_TEXT_BASELINE
            );
            display.print(bottomActions[actionIndex]);
            actionTop += Theme::SELECT_LIST_ITEM_HEIGHT +
                Theme::SELECT_LIST_ITEM_GAP;
        }
    }
}

void redrawSelectListAfterMove(
    const char* const items[],
    uint8_t itemCount,
    const SelectListState& previousState,
    SelectListState& state,
    SelectListConfig config,
    const char* const bottomActions[]
) {
    if (
        items == nullptr || itemCount == 0 ||
        previousState.selectedIndex == state.selectedIndex
    ) {
        return;
    }

    if (config.bottomActionCount > 0)
    {
        const int listTop = getListTop();
        const int listBottom = display.height() - Theme::FOOTER_HEIGHT -
            Theme::SELECT_LIST_VERTICAL_PADDING;
        display.setPartialWindow(
            Theme::PAGE_MARGIN,
            listTop,
            display.width() - (Theme::PAGE_MARGIN * 2),
            listBottom - listTop
        );
        display.firstPage();
        do
        {
            display.fillRect(
                Theme::PAGE_MARGIN,
                listTop,
                display.width() - (Theme::PAGE_MARGIN * 2),
                listBottom - listTop,
                Theme::BACKGROUND_COLOR
            );
            drawSelectList(
                items, itemCount, state, config, bottomActions
            );
        }
        while (display.nextPage());
        return;
    }

    keepSelectionVisible(items, itemCount, state, config);
    const int contentLeft = Theme::PAGE_MARGIN;
    const int contentWidth = display.width() - (Theme::PAGE_MARGIN * 2);

    if (previousState.firstVisibleIndex != state.firstVisibleIndex)
    {
        const int listTop = getListTop();
        const int listHeight = getListBottom(config) - listTop;
        display.setPartialWindow(contentLeft, listTop, contentWidth, listHeight);
        display.firstPage();
        do
        {
            display.fillRect(
                contentLeft, listTop, contentWidth, listHeight,
                Theme::BACKGROUND_COLOR
            );
            drawSelectList(items, itemCount, state, config);
        }
        while (display.nextPage());
        return;
    }

    const uint16_t maximumTextWidth = getMaximumTextWidth();
    const int previousTop = getRowTop(
        items, previousState.selectedIndex, state, config, maximumTextWidth
    );
    const int selectedTop = getRowTop(
        items, state.selectedIndex, state, config, maximumTextWidth
    );
    const int previousBottom = previousTop + getItemHeight(
        items[previousState.selectedIndex], config, maximumTextWidth
    );
    const int selectedBottom = selectedTop + getItemHeight(
        items[state.selectedIndex], config, maximumTextWidth
    );
    const int updateTop = min(previousTop, selectedTop);
    const int updateBottom = max(previousBottom, selectedBottom);

    display.setPartialWindow(
        contentLeft,
        updateTop,
        Theme::SELECT_LIST_MARKER_WIDTH,
        updateBottom - updateTop
    );
    display.firstPage();
    do
    {
        display.fillRect(
            contentLeft,
            updateTop,
            Theme::SELECT_LIST_MARKER_WIDTH,
            updateBottom - updateTop,
            Theme::BACKGROUND_COLOR
        );
        display.fillRect(
            contentLeft,
            selectedTop,
            Theme::SELECT_LIST_MARKER_WIDTH,
            selectedBottom - selectedTop,
            Theme::TEXT_COLOR
        );
    }
    while (display.nextPage());
}

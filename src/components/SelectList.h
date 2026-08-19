#pragma once

#include <Arduino.h>

struct SelectListState
{
    uint8_t selectedIndex = 0;
    uint8_t firstVisibleIndex = 0;
};

struct SelectListConfig
{
    constexpr SelectListConfig(
        uint8_t nextTextLineCount = 1,
        uint8_t nextBottomActionCount = 0
    ) :
        textLineCount(nextTextLineCount),
        bottomActionCount(nextBottomActionCount)
    {
    }

    uint8_t textLineCount;
    uint8_t bottomActionCount;
};

constexpr SelectListConfig TWO_LINE_SELECT_LIST(2);
constexpr SelectListConfig TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTION(2, 1);
constexpr SelectListConfig TWO_LINE_SELECT_LIST_WITH_BOTTOM_ACTIONS(2, 2);

void resetSelectList(SelectListState& state);

void moveSelectListUp(
    SelectListState& state,
    uint8_t itemCount
);

void moveSelectListDown(
    SelectListState& state,
    uint8_t itemCount
);

void drawSelectList(
    const char* const items[],
    uint8_t itemCount,
    SelectListState& state,
    SelectListConfig config = SelectListConfig(),
    const char* const bottomActions[] = nullptr
);

void redrawSelectListAfterMove(
    const char* const items[],
    uint8_t itemCount,
    const SelectListState& previousState,
    SelectListState& state,
    SelectListConfig config = SelectListConfig(),
    const char* const bottomActions[] = nullptr
);

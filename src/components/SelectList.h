#pragma once

#include <Arduino.h>

struct SelectListState
{
    uint8_t selectedIndex = 0;
    uint8_t firstVisibleIndex = 0;
};

struct SelectListConfig
{
    constexpr SelectListConfig(uint8_t nextTextLineCount = 1)
        : textLineCount(nextTextLineCount)
    {
    }

    uint8_t textLineCount;
};

constexpr SelectListConfig TWO_LINE_SELECT_LIST(2);

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
    SelectListConfig config = SelectListConfig()
);

void redrawSelectListAfterMove(
    const char* const items[],
    uint8_t itemCount,
    const SelectListState& previousState,
    SelectListState& state,
    SelectListConfig config = SelectListConfig()
);

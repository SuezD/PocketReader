#pragma once

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

namespace Theme
{
    // Colours
    constexpr uint16_t BACKGROUND_COLOR = GxEPD_WHITE;
    constexpr uint16_t TEXT_COLOR = GxEPD_BLACK;
    constexpr uint16_t BORDER_COLOR = GxEPD_BLACK;

    // Fonts
    constexpr const GFXfont* HEADER_FONT = &FreeMonoBold9pt7b;
    constexpr const GFXfont* BODY_FONT = &FreeMono9pt7b;
    constexpr const GFXfont* FOOTER_FONT = &FreeMono9pt7b;

    // Layout
    constexpr int PAGE_MARGIN = 12;
    constexpr int HEADER_HEIGHT = 36;
    constexpr int HEADER_TEXT_BASELINE = 24;
    constexpr int HEADER_TEXT_GAP = 12;
    constexpr int FOOTER_HEIGHT = 32;
    constexpr int FOOTER_TEXT_BASELINE = 22;
    constexpr int FOOTER_TEXT_GAP = 12;

    constexpr int MESSAGE_LINE_HEIGHT = 24;
    constexpr int MESSAGE_TEXT_BASELINE = 18;

    constexpr int MESSAGE_OPTIONS_TOP_GAP = 20;
    constexpr int MESSAGE_OPTION_HEIGHT = 32;
    constexpr int MESSAGE_OPTION_GAP = 8;
    constexpr int MESSAGE_OPTION_TEXT_BASELINE = 22;
    constexpr int MESSAGE_OPTION_HORIZONTAL_PADDING = 8;

    constexpr int SELECT_LIST_VERTICAL_PADDING = 12;
    constexpr int SELECT_LIST_ITEM_HEIGHT = 34;
    constexpr int SELECT_LIST_ITEM_GAP = 6;
    constexpr int SELECT_LIST_TEXT_BASELINE = 23;
    constexpr int SELECT_LIST_HORIZONTAL_PADDING = 8;
}

#include "Header.h"

#include "Display.h"
#include "Theme.h"
#include "helpers/TextUtils.h"

namespace
{
    constexpr size_t TITLE_BUFFER_SIZE = 64;
}

void drawHeader(
    const char* leftText,
    uint8_t batteryPercent
) {
    display.setFont(Theme::HEADER_FONT);
    display.setTextColor(Theme::TEXT_COLOR);

    char batteryText[5];

    snprintf(
        batteryText,
        sizeof(batteryText),
        "%u%%",
        batteryPercent
    );

    const uint16_t batteryWidth = getTextWidth(
        batteryText,
        Theme::HEADER_FONT
    );

    const int contentLeft = Theme::PAGE_MARGIN;
    const int contentRight =
        display.width() - Theme::PAGE_MARGIN;

    // The battery always receives all the space it needs.
    const int batteryX =
        contentRight - batteryWidth;

    const uint16_t leftMaximumWidth =
        batteryX -
        Theme::HEADER_TEXT_GAP -
        contentLeft;

    char leftOutput[TITLE_BUFFER_SIZE];

    truncateToWidth(
        leftText,
        leftOutput,
        sizeof(leftOutput),
        leftMaximumWidth,
        Theme::HEADER_FONT
    );

    if (leftOutput[0] != '\0') {
        display.setCursor(
            contentLeft,
            Theme::HEADER_TEXT_BASELINE
        );

        display.print(leftOutput);
    }

    // Always draw the battery.
    display.setCursor(
        batteryX,
        Theme::HEADER_TEXT_BASELINE
    );

    display.print(batteryText);

    display.drawLine(
        0,
        Theme::HEADER_HEIGHT,
        display.width() - 1,
        Theme::HEADER_HEIGHT,
        Theme::BORDER_COLOR
    );
}

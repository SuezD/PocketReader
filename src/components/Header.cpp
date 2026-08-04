#include "Header.h"
#include "Display.h"
#include "Theme.h"

#include <Fonts/FreeMonoBold9pt7b.h>

void drawHeader(const char* title, uint8_t batteryPercent)
{
    display.setTextColor(Theme::TEXT_COLOR);
    display.setFont(Theme::HEADER_FONT);

    display.setCursor(
        Theme::PAGE_MARGIN,
        Theme::HEADER_TEXT_BASELINE
    );
    display.print(title);

    char batteryText[5];
    snprintf(batteryText, sizeof(batteryText), "%u%%", batteryPercent);

    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;

    display.getTextBounds(
        batteryText,
        0,
        0,
        &x,
        &y,
        &width,
        &height
    );

    display.setCursor(
        display.width() - Theme::PAGE_MARGIN - width,
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

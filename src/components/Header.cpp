#include "Header.h"

#include "Display.h"
#include "Theme.h"
#include "helpers/TextUtils.h"
#include "wifi/WifiService.h"

namespace
{
    constexpr size_t TITLE_BUFFER_SIZE = 64;
    constexpr int WIFI_ICON_WIDTH = 16;
    constexpr int WIFI_ICON_HEIGHT = 14;

    void drawWifiIcon(int x, int y)
    {
        const int centerX = x + WIFI_ICON_WIDTH / 2;

        display.fillCircle(centerX, y + 11, 1, Theme::TEXT_COLOR);
        display.drawLine(centerX - 3, y + 8, centerX, y + 6, Theme::TEXT_COLOR);
        display.drawLine(centerX, y + 6, centerX + 3, y + 8, Theme::TEXT_COLOR);
        display.drawLine(centerX - 6, y + 5, centerX, y + 2, Theme::TEXT_COLOR);
        display.drawLine(centerX, y + 2, centerX + 6, y + 5, Theme::TEXT_COLOR);

        if (!getWifiManager().isConnected())
        {
            display.drawLine(x + 2, y, x + 14, y + 13, Theme::TEXT_COLOR);
            display.drawLine(x + 3, y, x + 15, y + 13, Theme::TEXT_COLOR);
        }
    }

    int getStatusLeft(uint16_t batteryWidth)
    {
        const int contentRight = display.width() - Theme::PAGE_MARGIN;
        const int batteryX = contentRight - batteryWidth;
        return batteryX - Theme::HEADER_TEXT_GAP - WIFI_ICON_WIDTH;
    }

    void drawStatus(uint8_t batteryPercent)
    {
        char batteryText[5];
        snprintf(batteryText, sizeof(batteryText), "%u%%", batteryPercent);

        const uint16_t batteryWidth = getTextWidth(
            batteryText,
            Theme::HEADER_FONT
        );
        const int contentRight = display.width() - Theme::PAGE_MARGIN;
        const int batteryX = contentRight - batteryWidth;
        const int wifiX = getStatusLeft(batteryWidth);

        drawWifiIcon(
            wifiX,
            Theme::HEADER_TEXT_BASELINE - WIFI_ICON_HEIGHT + 1
        );
        display.setFont(Theme::HEADER_FONT);
        display.setTextColor(Theme::TEXT_COLOR);
        display.setCursor(batteryX, Theme::HEADER_TEXT_BASELINE);
        display.print(batteryText);
    }
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
    const int statusLeft = getStatusLeft(batteryWidth);

    const uint16_t leftMaximumWidth =
        statusLeft -
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

    drawStatus(batteryPercent);

    display.drawLine(
        0,
        Theme::HEADER_HEIGHT,
        display.width() - 1,
        Theme::HEADER_HEIGHT,
        Theme::BORDER_COLOR
    );
}

void redrawHeaderStatus(uint8_t batteryPercent)
{
    display.setFont(Theme::HEADER_FONT);

    char batteryText[5];
    snprintf(batteryText, sizeof(batteryText), "%u%%", batteryPercent);
    const uint16_t batteryWidth = getTextWidth(
        batteryText,
        Theme::HEADER_FONT
    );
    const int statusLeft = getStatusLeft(batteryWidth);
    const int updateLeft = statusLeft - (statusLeft % 8);
    const int updateWidth = display.width() - updateLeft;

    display.setPartialWindow(
        updateLeft,
        0,
        updateWidth,
        Theme::HEADER_HEIGHT + 1
    );
    display.firstPage();

    do
    {
        display.fillRect(
            updateLeft,
            0,
            updateWidth,
            Theme::HEADER_HEIGHT,
            Theme::BACKGROUND_COLOR
        );
        drawStatus(batteryPercent);
        display.drawLine(
            updateLeft,
            Theme::HEADER_HEIGHT,
            display.width() - 1,
            Theme::HEADER_HEIGHT,
            Theme::BORDER_COLOR
        );
    }
    while (display.nextPage());
}

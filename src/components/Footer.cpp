#include "Footer.h"

#include <string.h>

#include "Display.h"
#include "Theme.h"
#include "helpers/TextUtils.h"

namespace
{
    constexpr size_t TEXT_BUFFER_SIZE = 64;
}

void drawFooter(
    const char* leftText,
    const char* rightText
) {
    const int footerTop =
        display.height() - Theme::FOOTER_HEIGHT;

    // The footer always displays, even without text.
    display.drawLine(
        0,
        footerTop,
        display.width() - 1,
        footerTop,
        Theme::BORDER_COLOR
    );

    display.setFont(Theme::BODY_FONT);
    display.setTextColor(Theme::TEXT_COLOR);

    const int contentLeft = Theme::PAGE_MARGIN;
    const int contentRight =
        display.width() - Theme::PAGE_MARGIN;

    const uint16_t totalAvailableWidth =
        contentRight - contentLeft;

    const bool hasLeftText =
        leftText != nullptr && leftText[0] != '\0';

    const bool hasRightText =
        rightText != nullptr && rightText[0] != '\0';

    uint16_t leftMaximumWidth = totalAvailableWidth;
    uint16_t rightMaximumWidth = totalAvailableWidth;

    if (hasLeftText && hasRightText) {
        const uint16_t availableTextWidth =
            totalAvailableWidth - Theme::FOOTER_TEXT_GAP;

        const uint16_t leftNaturalWidth =
            getTextWidth(leftText, Theme::BODY_FONT);

        const uint16_t rightNaturalWidth =
            getTextWidth(rightText, Theme::BODY_FONT);

        const uint16_t halfWidth =
            availableTextWidth / 2;

        // Let a short label keep its natural width and give the
        // remaining space to the longer label.
        if (leftNaturalWidth <= halfWidth) {
            leftMaximumWidth = leftNaturalWidth;
            rightMaximumWidth =
                availableTextWidth - leftMaximumWidth;
        }
        else if (rightNaturalWidth <= halfWidth) {
            rightMaximumWidth = rightNaturalWidth;
            leftMaximumWidth =
                availableTextWidth - rightMaximumWidth;
        }
        else {
            leftMaximumWidth = halfWidth;
            rightMaximumWidth =
                availableTextWidth - halfWidth;
        }
    }

    char leftOutput[TEXT_BUFFER_SIZE];
    char rightOutput[TEXT_BUFFER_SIZE];

    truncateToWidth(
        leftText,
        leftOutput,
        sizeof(leftOutput),
        leftMaximumWidth,
        Theme::BODY_FONT
    );

    truncateToWidth(
        rightText,
        rightOutput,
        sizeof(rightOutput),
        rightMaximumWidth,
        Theme::BODY_FONT
    );

    const int textBaseline =
        footerTop + Theme::FOOTER_TEXT_BASELINE;

    if (leftOutput[0] != '\0') {
        display.setCursor(contentLeft, textBaseline);
        display.print(leftOutput);
    }

    if (rightOutput[0] != '\0') {
        const uint16_t rightTextWidth =
            getTextWidth(rightOutput, Theme::BODY_FONT);

        display.setCursor(
            contentRight - rightTextWidth,
            textBaseline
        );

        display.print(rightOutput);
    }
}

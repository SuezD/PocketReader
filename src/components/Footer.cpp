#include "Footer.h"

#include <string.h>

#include "Display.h"
#include "Theme.h"

namespace
{
    constexpr size_t TEXT_BUFFER_SIZE = 64;

    uint16_t getTextWidth(const char* text)
    {
        int16_t x;
        int16_t y;
        uint16_t width;
        uint16_t height;

        display.getTextBounds(
            text,
            0,
            0,
            &x,
            &y,
            &width,
            &height
        );

        return width;
    }

    void truncateToWidth(
        const char* source,
        char* output,
        size_t outputSize,
        uint16_t maximumWidth
    ) {
        output[0] = '\0';

        if (source == nullptr || source[0] == '\0') {
            return;
        }

        const size_t sourceLength = strlen(source);
        size_t copiedLength = sourceLength;

        if (copiedLength >= outputSize) {
            copiedLength = outputSize - 1;
        }

        memcpy(output, source, copiedLength);
        output[copiedLength] = '\0';

        const bool wholeTextCopied =
            copiedLength == sourceLength;

        if (
            wholeTextCopied &&
            getTextWidth(output) <= maximumWidth
        ) {
            return;
        }

        constexpr char ELLIPSIS[] = "...";
        const uint16_t ellipsisWidth =
            getTextWidth(ELLIPSIS);

        if (ellipsisWidth > maximumWidth) {
            output[0] = '\0';
            return;
        }

        while (
            copiedLength > 0 &&
            (
                copiedLength + 3 >= outputSize ||
                getTextWidth(output) + ellipsisWidth >
                    maximumWidth
            )
        ) {
            copiedLength--;
            output[copiedLength] = '\0';
        }

        strcat(output, ELLIPSIS);
    }
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
            getTextWidth(leftText);

        const uint16_t rightNaturalWidth =
            getTextWidth(rightText);

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
        leftMaximumWidth
    );

    truncateToWidth(
        rightText,
        rightOutput,
        sizeof(rightOutput),
        rightMaximumWidth
    );

    const int textBaseline =
        footerTop + Theme::FOOTER_TEXT_BASELINE;

    if (leftOutput[0] != '\0') {
        display.setCursor(contentLeft, textBaseline);
        display.print(leftOutput);
    }

    if (rightOutput[0] != '\0') {
        const uint16_t rightTextWidth =
            getTextWidth(rightOutput);

        display.setCursor(
            contentRight - rightTextWidth,
            textBaseline
        );

        display.print(rightOutput);
    }
}

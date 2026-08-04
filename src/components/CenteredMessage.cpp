#include "CenteredMessage.h"

#include "Display.h"
#include "Theme.h"
#include "helpers/TextUtils.h"

namespace
{
    constexpr size_t TEXT_BUFFER_SIZE = 64;

    bool hasText(const char* text)
    {
        return text != nullptr && text[0] != '\0';
    }

    void drawCenteredLine(
        const char* text,
        int baseline,
        uint16_t maximumWidth
    ) {
        char output[TEXT_BUFFER_SIZE];

        truncateToWidth(
            text,
            output,
            sizeof(output),
            maximumWidth,
            Theme::BODY_FONT
        );

        if (output[0] == '\0')
        {
            return;
        }

        const uint16_t textWidth = getTextWidth(
            output,
            Theme::BODY_FONT
        );

        const int textX =
            (display.width() - textWidth) / 2;

        display.setFont(Theme::BODY_FONT);
        display.setTextColor(Theme::TEXT_COLOR);
        display.setCursor(textX, baseline);
        display.print(output);
    }
}

void drawMessage(
    const char* firstLine,
    const char* secondLine,
    const char* const options[],
    uint8_t optionCount,
    uint8_t selectedIndex
) {
    const int contentLeft = Theme::PAGE_MARGIN;
    const int contentRight =
        display.width() - Theme::PAGE_MARGIN;

    const int contentTop =
        Theme::HEADER_HEIGHT + 1;

    const int contentBottom =
        display.height() - Theme::FOOTER_HEIGHT;

    const uint16_t contentWidth =
        contentRight - contentLeft;

    const int contentHeight =
        contentBottom - contentTop;

    const bool hasFirstLine = hasText(firstLine);
    const bool hasSecondLine = hasText(secondLine);

    const uint8_t lineCount =
        static_cast<uint8_t>(hasFirstLine) +
        static_cast<uint8_t>(hasSecondLine);

    int blockHeight =
        lineCount * Theme::MESSAGE_LINE_HEIGHT;

    if (optionCount > 0)
    {
        if (lineCount > 0)
        {
            blockHeight += Theme::MESSAGE_OPTIONS_TOP_GAP;
        }

        blockHeight +=
            optionCount * Theme::MESSAGE_OPTION_HEIGHT;

        if (optionCount > 1)
        {
            blockHeight +=
                (optionCount - 1) *
                Theme::MESSAGE_OPTION_GAP;
        }
    }

    int blockTop = contentTop;

    if (blockHeight < contentHeight)
    {
        blockTop +=
            (contentHeight - blockHeight) / 2;
    }

    int currentY = blockTop;

    if (hasFirstLine)
    {
        drawCenteredLine(
            firstLine,
            currentY + Theme::MESSAGE_TEXT_BASELINE,
            contentWidth
        );

        currentY += Theme::MESSAGE_LINE_HEIGHT;
    }

    if (hasSecondLine)
    {
        drawCenteredLine(
            secondLine,
            currentY + Theme::MESSAGE_TEXT_BASELINE,
            contentWidth
        );

        currentY += Theme::MESSAGE_LINE_HEIGHT;
    }

    if (optionCount == 0 || options == nullptr)
    {
        return;
    }

    if (lineCount > 0)
    {
        currentY += Theme::MESSAGE_OPTIONS_TOP_GAP;
    }

    uint16_t widestOption = 0;

    for (uint8_t i = 0; i < optionCount; i++)
    {
        const uint16_t optionWidth =
            getTextWidth(options[i], Theme::BODY_FONT);

        if (optionWidth > widestOption)
        {
            widestOption = optionWidth;
        }
    }

    uint16_t optionBoxWidth =
        widestOption +
        (Theme::MESSAGE_OPTION_HORIZONTAL_PADDING * 2);

    if (optionBoxWidth > contentWidth)
    {
        optionBoxWidth = contentWidth;
    }

    const int optionBoxX =
        (display.width() - optionBoxWidth) / 2;

    const uint16_t optionTextWidth =
        optionBoxWidth -
        (Theme::MESSAGE_OPTION_HORIZONTAL_PADDING * 2);

    const uint8_t safeSelectedIndex =
        selectedIndex < optionCount
            ? selectedIndex
            : 0;

    for (uint8_t i = 0; i < optionCount; i++)
    {
        const bool isSelected =
            i == safeSelectedIndex;

        if (isSelected)
        {
            display.fillRect(
                optionBoxX,
                currentY,
                optionBoxWidth,
                Theme::MESSAGE_OPTION_HEIGHT,
                Theme::TEXT_COLOR
            );
        }

        char output[TEXT_BUFFER_SIZE];

        truncateToWidth(
            options[i],
            output,
            sizeof(output),
            optionTextWidth,
            Theme::BODY_FONT
        );

        const uint16_t textWidth =
            getTextWidth(output, Theme::BODY_FONT);

        const int textX =
            (display.width() - textWidth) / 2;

        display.setFont(Theme::BODY_FONT);
        display.setTextColor(
            isSelected
                ? Theme::BACKGROUND_COLOR
                : Theme::TEXT_COLOR
        );

        display.setCursor(
            textX,
            currentY +
                Theme::MESSAGE_OPTION_TEXT_BASELINE
        );

        display.print(output);

        currentY +=
            Theme::MESSAGE_OPTION_HEIGHT +
            Theme::MESSAGE_OPTION_GAP;
    }

    display.setTextColor(Theme::TEXT_COLOR);
}

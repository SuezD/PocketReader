#include "CenteredMessage.h"

#include "Display.h"
#include "DisplayRefresh.h"
#include "Theme.h"
#include "helpers/TextUtils.h"

namespace
{
    constexpr size_t TEXT_BUFFER_SIZE = 64;

    struct MessageSelectionLayout
    {
        int optionsTop;
        int optionGroupX;
    };

    bool hasText(const char* text)
    {
        return text != nullptr && text[0] != '\0';
    }

    bool isInlineWhitespace(char character)
    {
        return character == ' ' || character == '\t';
    }

    uint8_t getMaximumCharactersPerLine(uint16_t maximumWidth)
    {
        const uint8_t glyphIndex =
            'M' - Theme::BODY_FONT->first;
        const uint8_t characterWidth =
            Theme::BODY_FONT->glyph[glyphIndex].xAdvance;
        uint8_t characterCount = maximumWidth / characterWidth;

        if (characterCount >= TEXT_BUFFER_SIZE)
        {
            characterCount = TEXT_BUFFER_SIZE - 1;
        }

        return characterCount;
    }

    const char* readWrappedLine(
        const char* source,
        char* output,
        uint16_t maximumWidth
    ) {
        output[0] = '\0';

        if (source == nullptr || source[0] == '\0')
        {
            return source;
        }

        if (source[0] == '\n')
        {
            return source + 1;
        }

        while (isInlineWhitespace(source[0]))
        {
            source++;
        }

        const uint8_t maximumCharacters =
            getMaximumCharactersPerLine(maximumWidth);
        uint8_t outputLength = 0;
        const char* position = source;

        while (position[0] != '\0')
        {
            if (position[0] == '\n')
            {
                output[outputLength] = '\0';
                return position + 1;
            }

            if (isInlineWhitespace(position[0]))
            {
                position++;
                continue;
            }

            const char* wordStart = position;
            uint8_t wordLength = 0;

            while (
                position[0] != '\0' &&
                position[0] != '\n' &&
                !isInlineWhitespace(position[0])
            ) {
                wordLength++;
                position++;
            }

            const uint8_t separatorLength =
                outputLength == 0 ? 0 : 1;

            if (
                outputLength > 0 &&
                outputLength + separatorLength + wordLength >
                    maximumCharacters
            ) {
                output[outputLength] = '\0';
                return wordStart;
            }

            if (outputLength == 0 && wordLength > maximumCharacters)
            {
                for (
                    uint8_t index = 0;
                    index < maximumCharacters;
                    index++
                ) {
                    output[index] = wordStart[index];
                }

                output[maximumCharacters] = '\0';
                return wordStart + maximumCharacters;
            }

            if (separatorLength > 0)
            {
                output[outputLength] = ' ';
                outputLength++;
            }

            for (uint8_t index = 0; index < wordLength; index++)
            {
                output[outputLength] = wordStart[index];
                outputLength++;
            }
        }

        output[outputLength] = '\0';
        return position;
    }

    uint8_t countWrappedLines(
        const char* text,
        uint16_t maximumWidth
    ) {
        if (!hasText(text))
        {
            return 0;
        }

        char output[TEXT_BUFFER_SIZE];
        const char* position = text;
        uint8_t lineCount = 0;

        while (position[0] != '\0')
        {
            position = readWrappedLine(
                position,
                output,
                maximumWidth
            );
            lineCount++;
        }

        return lineCount;
    }

    void drawCenteredWrappedText(
        const char* text,
        int& currentY,
        uint16_t maximumWidth
    ) {
        char output[TEXT_BUFFER_SIZE];
        const char* position = text;

        while (position[0] != '\0')
        {
            position = readWrappedLine(
                position,
                output,
                maximumWidth
            );

            if (output[0] != '\0')
            {
                const uint16_t textWidth = getTextWidth(
                    output,
                    Theme::BODY_FONT
                );

                const int textX =
                    (display.width() - textWidth) / 2;

                display.setFont(Theme::BODY_FONT);
                display.setTextColor(Theme::TEXT_COLOR);
                display.setCursor(
                    textX,
                    currentY + Theme::MESSAGE_TEXT_BASELINE
                );
                display.print(output);
            }

            currentY += Theme::MESSAGE_LINE_HEIGHT;
        }
    }

    MessageSelectionLayout calculateSelectionLayout(
        const char* firstLine,
        const char* secondLine,
        const char* const options[],
        uint8_t optionCount
    ) {
        const uint16_t contentWidth =
            display.width() - 2 * Theme::PAGE_MARGIN;
        const int contentTop = Theme::HEADER_HEIGHT + 1;
        const int contentHeight =
            display.height() - Theme::FOOTER_HEIGHT - contentTop;
        const uint8_t lineCount =
            countWrappedLines(firstLine, contentWidth) +
            countWrappedLines(secondLine, contentWidth);
        int blockHeight = lineCount * Theme::MESSAGE_LINE_HEIGHT;

        if (optionCount > 0)
        {
            if (lineCount > 0)
            {
                blockHeight += Theme::MESSAGE_OPTIONS_TOP_GAP;
            }
            blockHeight += optionCount * Theme::MESSAGE_OPTION_HEIGHT;
            if (optionCount > 1)
            {
                blockHeight +=
                    (optionCount - 1) * Theme::MESSAGE_OPTION_GAP;
            }
        }

        int optionsTop = contentTop;
        if (blockHeight < contentHeight)
        {
            optionsTop += (contentHeight - blockHeight) / 2;
        }
        optionsTop += lineCount * Theme::MESSAGE_LINE_HEIGHT;
        if (lineCount > 0 && optionCount > 0)
        {
            optionsTop += Theme::MESSAGE_OPTIONS_TOP_GAP;
        }

        uint16_t widestOption = 0;
        for (uint8_t i = 0; options != nullptr && i < optionCount; i++)
        {
            const uint16_t width =
                getTextWidth(options[i], Theme::BODY_FONT);
            if (width > widestOption)
            {
                widestOption = width;
            }
        }
        const uint16_t markerAndGapWidth =
            Theme::SELECT_LIST_MARKER_WIDTH +
            Theme::SELECT_LIST_MARKER_GAP;
        const uint16_t optionTextWidth = min(
            widestOption,
            static_cast<uint16_t>(contentWidth - markerAndGapWidth)
        );

        return {
            optionsTop,
            (display.width() - markerAndGapWidth - optionTextWidth) / 2
        };
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

    const uint8_t firstLineCount =
        countWrappedLines(firstLine, contentWidth);
    const uint8_t secondLineCount =
        countWrappedLines(secondLine, contentWidth);

    const uint8_t lineCount = firstLineCount + secondLineCount;

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
        drawCenteredWrappedText(
            firstLine,
            currentY,
            contentWidth
        );
    }

    if (hasSecondLine)
    {
        drawCenteredWrappedText(
            secondLine,
            currentY,
            contentWidth
        );
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

    const uint16_t markerAndGapWidth =
        Theme::SELECT_LIST_MARKER_WIDTH +
        Theme::SELECT_LIST_MARKER_GAP;

    const uint16_t maximumOptionTextWidth =
        contentWidth - markerAndGapWidth;

    const uint16_t optionTextWidth =
        min(widestOption, maximumOptionTextWidth);

    const int optionGroupX =
        (display.width() -
            (markerAndGapWidth + optionTextWidth)) /
        2;

    const uint8_t safeSelectedIndex =
        selectedIndex < optionCount
            ? selectedIndex
            : 0;

    for (uint8_t i = 0; i < optionCount; i++)
    {
        const bool isSelected =
            i == safeSelectedIndex;

        char output[TEXT_BUFFER_SIZE];

        truncateToWidth(
            options[i],
            output,
            sizeof(output),
            optionTextWidth,
            Theme::BODY_FONT
        );

        if (isSelected)
        {
            display.fillRect(
                optionGroupX,
                currentY,
                Theme::SELECT_LIST_MARKER_WIDTH,
                Theme::MESSAGE_OPTION_HEIGHT,
                Theme::TEXT_COLOR
            );
        }

        display.setFont(Theme::BODY_FONT);
        display.setTextColor(Theme::TEXT_COLOR);

        display.setCursor(
            optionGroupX + markerAndGapWidth,
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

void redrawMessageSelection(
    const char* firstLine,
    const char* secondLine,
    const char* const options[],
    uint8_t optionCount,
    uint8_t previousIndex,
    uint8_t selectedIndex
) {
    if (
        options == nullptr || optionCount == 0 ||
        previousIndex >= optionCount || selectedIndex >= optionCount ||
        previousIndex == selectedIndex
    ) {
        return;
    }

    const MessageSelectionLayout layout = calculateSelectionLayout(
        firstLine, secondLine, options, optionCount
    );
    const int optionStride =
        Theme::MESSAGE_OPTION_HEIGHT + Theme::MESSAGE_OPTION_GAP;
    const uint8_t firstIndex = min(previousIndex, selectedIndex);
    const uint8_t lastIndex = max(previousIndex, selectedIndex);
    const int updateTop = layout.optionsTop + firstIndex * optionStride;
    const int updateHeight =
        (lastIndex - firstIndex) * optionStride +
        Theme::MESSAGE_OPTION_HEIGHT;

    setPartialRefreshWindow(
        layout.optionGroupX,
        updateTop,
        Theme::SELECT_LIST_MARKER_WIDTH,
        updateHeight
    );
    display.firstPage();
    do
    {
        display.fillRect(
            layout.optionGroupX,
            updateTop,
            Theme::SELECT_LIST_MARKER_WIDTH,
            updateHeight,
            Theme::BACKGROUND_COLOR
        );
        display.fillRect(
            layout.optionGroupX,
            layout.optionsTop + selectedIndex * optionStride,
            Theme::SELECT_LIST_MARKER_WIDTH,
            Theme::MESSAGE_OPTION_HEIGHT,
            Theme::TEXT_COLOR
        );
    }
    while (display.nextPage());
}

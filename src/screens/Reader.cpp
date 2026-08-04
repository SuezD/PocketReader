#include "screens/Reader.h"

#include <stdio.h>

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"

namespace
{
    constexpr uint8_t READER_VERTICAL_PADDING = 8;
    constexpr uint8_t READER_LINE_HEIGHT = 18;
    constexpr uint8_t READER_TEXT_BASELINE = 13;
    constexpr uint8_t MAX_LINE_LENGTH = 48;

    const CachedBook* currentBook = nullptr;
    const char* currentText = nullptr;
    uint16_t currentPage = 0;

    bool isInlineWhitespace(char character)
    {
        return character == ' ' || character == '\t';
    }

    uint8_t getMaximumCharactersPerLine()
    {
        const uint8_t glyphIndex =
            'M' - Theme::BODY_FONT->first;
        const uint8_t characterWidth =
            Theme::BODY_FONT->glyph[glyphIndex].xAdvance;

        const int contentWidth =
            display.width() - (Theme::PAGE_MARGIN * 2);

        uint8_t characterCount = contentWidth / characterWidth;

        if (characterCount >= MAX_LINE_LENGTH)
        {
            characterCount = MAX_LINE_LENGTH - 1;
        }

        return characterCount;
    }

    const char* readNextLine(
        const char* source,
        char* output
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
            getMaximumCharactersPerLine();
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

    uint8_t getLinesPerPage()
    {
        const int contentHeight =
            display.height() -
            Theme::HEADER_HEIGHT -
            Theme::FOOTER_HEIGHT -
            (READER_VERTICAL_PADDING * 2);

        return contentHeight / READER_LINE_HEIGHT;
    }

    const char* getNextPageStart(const char* pageStart)
    {
        char line[MAX_LINE_LENGTH];
        const char* position = pageStart;

        for (
            uint8_t lineIndex = 0;
            lineIndex < getLinesPerPage() && position[0] != '\0';
            lineIndex++
        ) {
            position = readNextLine(position, line);
        }

        return position;
    }

    const char* getPageStart(uint16_t pageIndex)
    {
        const char* pageStart = currentText;

        for (
            uint16_t index = 0;
            index < pageIndex && pageStart[0] != '\0';
            index++
        ) {
            pageStart = getNextPageStart(pageStart);
        }

        return pageStart;
    }

    uint16_t getPageCount()
    {
        if (currentText == nullptr || currentText[0] == '\0')
        {
            return 0;
        }

        uint16_t count = 1;
        const char* pageStart = currentText;

        while (true)
        {
            const char* nextPageStart =
                getNextPageStart(pageStart);

            if (nextPageStart[0] == '\0')
            {
                return count;
            }

            count++;
            pageStart = nextPageStart;
        }
    }

    void drawCurrentTextPage()
    {
        const int contentTop =
            Theme::HEADER_HEIGHT + READER_VERTICAL_PADDING;
        const int contentBottom =
            display.height() -
            Theme::FOOTER_HEIGHT - READER_VERTICAL_PADDING;

        char line[MAX_LINE_LENGTH];
        const char* position = getPageStart(currentPage);

        display.setFont(Theme::BODY_FONT);
        display.setTextColor(Theme::TEXT_COLOR);

        for (
            int baseline = contentTop + READER_TEXT_BASELINE;
            baseline < contentBottom && position[0] != '\0';
            baseline += READER_LINE_HEIGHT
        ) {
            position = readNextLine(position, line);

            if (line[0] != '\0')
            {
                display.setCursor(Theme::PAGE_MARGIN, baseline);
                display.print(line);
            }
        }
    }
}

void openReader(const CachedBook* book, const char* text)
{
    currentBook = book;
    currentText = text;
    currentPage = 0;
}

bool moveReaderPreviousPage()
{
    if (currentBook == nullptr || currentPage == 0)
    {
        return false;
    }

    currentPage--;
    return true;
}

bool moveReaderNextPage()
{
    const uint16_t pageCount = getPageCount();

    if (pageCount == 0 || currentPage + 1 >= pageCount)
    {
        return false;
    }

    currentPage++;
    return true;
}

void drawReader(uint8_t batteryPercent)
{
    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);

        if (currentBook == nullptr || currentText == nullptr)
        {
            const char* options[] = {
                "My Books",
                "Add Books"
            };
            drawHeader("READER", batteryPercent);
            drawMessage("Nothing in Progress", nullptr, options, 2, 0);
            drawFooter();
            continue;
        }

        drawHeader(currentBook->title, batteryPercent);
        drawCurrentTextPage();

        char pageText[16];

        snprintf(
            pageText,
            sizeof(pageText),
            "Page %u of %u",
            currentPage + 1,
            getPageCount()
        );

        drawFooter("Chapter 1", pageText);
    }
    while (display.nextPage());
}

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
    constexpr uint8_t PARTIAL_TURNS_BEFORE_FULL_REFRESH = 12;
    constexpr int READER_PARTIAL_UPDATE_TOP =
        Theme::HEADER_HEIGHT + 1;

    const CachedBook* currentBook = nullptr;
    ReaderDocument currentDocument = {};
    uint16_t currentPage = 0;
    bool readerNeedsFullRefresh = true;
    uint8_t partialTurnsSinceFullRefresh = 0;

    bool hasOpenDocument()
    {
        return
            currentDocument.byteLength > 0 &&
            currentDocument.readCharacter != nullptr;
    }

    char readCharacter(uint32_t position)
    {
        if (!hasOpenDocument() || position >= currentDocument.byteLength)
        {
            return '\0';
        }

        return currentDocument.readCharacter(
            currentDocument.sourceContext,
            position
        );
    }

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

    uint32_t readNextLine(
        uint32_t source,
        char* output
    ) {
        output[0] = '\0';

        if (source >= currentDocument.byteLength)
        {
            return source;
        }

        if (readCharacter(source) == '\n')
        {
            return source + 1;
        }

        while (
            source < currentDocument.byteLength &&
            isInlineWhitespace(readCharacter(source))
        )
        {
            source++;
        }

        const uint8_t maximumCharacters =
            getMaximumCharactersPerLine();
        uint8_t outputLength = 0;
        uint32_t position = source;

        while (position < currentDocument.byteLength)
        {
            if (readCharacter(position) == '\n')
            {
                output[outputLength] = '\0';
                return position + 1;
            }

            if (isInlineWhitespace(readCharacter(position)))
            {
                position++;
                continue;
            }

            const uint32_t wordStart = position;
            uint8_t wordLength = 0;

            while (
                position < currentDocument.byteLength &&
                readCharacter(position) != '\n' &&
                !isInlineWhitespace(readCharacter(position))
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
                    output[index] = readCharacter(wordStart + index);
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
                output[outputLength] =
                    readCharacter(wordStart + index);
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

    uint32_t getNextPageStart(uint32_t pageStart)
    {
        char line[MAX_LINE_LENGTH];
        uint32_t position = pageStart;

        for (
            uint8_t lineIndex = 0;
            lineIndex < getLinesPerPage() &&
                position < currentDocument.byteLength;
            lineIndex++
        ) {
            position = readNextLine(position, line);
        }

        return position;
    }

    uint32_t getPageStart(uint16_t pageIndex)
    {
        uint32_t pageStart = 0;

        for (
            uint16_t index = 0;
            index < pageIndex && pageStart < currentDocument.byteLength;
            index++
        ) {
            pageStart = getNextPageStart(pageStart);
        }

        return pageStart;
    }

    uint16_t getPageCount()
    {
        if (!hasOpenDocument())
        {
            return 0;
        }

        uint16_t count = 1;
        uint32_t pageStart = 0;

        while (true)
        {
            const uint32_t nextPageStart =
                getNextPageStart(pageStart);

            if (nextPageStart >= currentDocument.byteLength)
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
        uint32_t position = getPageStart(currentPage);

        display.setFont(Theme::BODY_FONT);
        display.setTextColor(Theme::TEXT_COLOR);

        for (
            int baseline = contentTop + READER_TEXT_BASELINE;
            baseline < contentBottom &&
                position < currentDocument.byteLength;
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

void openReader(
    const CachedBook* book,
    const ReaderDocument& document
)
{
    currentBook = book;
    currentDocument = document;
    currentPage = 0;
    readerNeedsFullRefresh = true;
    partialTurnsSinceFullRefresh = 0;
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

void requestReaderFullRefresh()
{
    readerNeedsFullRefresh = true;
}

void drawReader(uint8_t batteryPercent)
{
    const bool useFullRefresh =
        readerNeedsFullRefresh ||
        partialTurnsSinceFullRefresh >=
            PARTIAL_TURNS_BEFORE_FULL_REFRESH;

    if (useFullRefresh)
    {
        display.setFullWindow();
    }
    else
    {
        display.setPartialWindow(
            0,
            READER_PARTIAL_UPDATE_TOP,
            display.width(),
            display.height() - READER_PARTIAL_UPDATE_TOP
        );
    }

    display.firstPage();

    do
    {
        if (useFullRefresh)
        {
            display.fillScreen(Theme::BACKGROUND_COLOR);
        }
        else
        {
            display.fillRect(
                0,
                READER_PARTIAL_UPDATE_TOP,
                display.width(),
                display.height() - READER_PARTIAL_UPDATE_TOP,
                Theme::BACKGROUND_COLOR
            );
        }

        if (currentBook == nullptr || !hasOpenDocument())
        {
            const char* options[] = {
                "My Books",
                "Add Books"
            };

            if (useFullRefresh)
            {
                drawHeader("READER", batteryPercent);
            }

            drawMessage("Nothing in Progress", nullptr, options, 2, 0);
            drawFooter();
            continue;
        }

        if (useFullRefresh)
        {
            drawHeader(currentBook->title, batteryPercent);
        }

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

    if (useFullRefresh)
    {
        readerNeedsFullRefresh = false;
        partialTurnsSinceFullRefresh = 0;
    }
    else
    {
        partialTurnsSinceFullRefresh++;
    }
}

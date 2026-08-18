#include "screens/Reader.h"

#include <stdio.h>
#include <stdlib.h>

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "navigation/PageRegistry.h"

namespace
{
    constexpr uint8_t READER_VERTICAL_PADDING = 8;
    constexpr uint8_t READER_LINE_HEIGHT = 18;
    constexpr uint8_t READER_TEXT_BASELINE = 13;
    constexpr uint8_t MAX_LINE_LENGTH = 48;
    constexpr uint8_t PARTIAL_TURNS_BEFORE_FULL_REFRESH = 12;
    constexpr uint16_t INITIAL_PAGE_INDEX_CAPACITY = 16;
    constexpr int READER_PARTIAL_UPDATE_TOP =
        Theme::HEADER_HEIGHT + 1;

    const CachedBook* currentBook = nullptr;
    ReaderDocument currentDocument = {};
    uint16_t currentPage = 0;
    uint8_t selectedEmptyOption = 0;
    bool readerNeedsFullRefresh = true;
    uint8_t partialTurnsSinceFullRefresh = 0;
    uint32_t* pageStartOffsets = nullptr;
    uint16_t pageStartCount = 0;
    uint16_t pageStartCapacity = 0;
    bool pageIndexReady = false;

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

    void clearPageIndex()
    {
        free(pageStartOffsets);
        pageStartOffsets = nullptr;
        pageStartCount = 0;
        pageStartCapacity = 0;
        pageIndexReady = false;
    }

    bool addPageStart(uint32_t pageStart)
    {
        if (pageStartCount == UINT16_MAX)
        {
            return false;
        }

        if (pageStartCount == pageStartCapacity)
        {
            uint32_t expandedCapacity =
                pageStartCapacity == 0 ?
                    INITIAL_PAGE_INDEX_CAPACITY :
                    static_cast<uint32_t>(pageStartCapacity) * 2;

            if (expandedCapacity > UINT16_MAX)
            {
                expandedCapacity = UINT16_MAX;
            }

            const size_t expandedBytes =
                expandedCapacity * sizeof(*pageStartOffsets);
            void* expandedOffsets =
                realloc(pageStartOffsets, expandedBytes);

            if (expandedOffsets == nullptr)
            {
                return false;
            }

            pageStartOffsets =
                static_cast<uint32_t*>(expandedOffsets);
            pageStartCapacity = expandedCapacity;
        }

        pageStartOffsets[pageStartCount] = pageStart;
        pageStartCount++;
        return true;
    }

    bool buildPageIndex()
    {
        clearPageIndex();

        if (!hasOpenDocument() || !addPageStart(0))
        {
            return false;
        }

        uint32_t pageStart = 0;

        while (pageStart < currentDocument.byteLength)
        {
            const uint32_t nextPageStart =
                getNextPageStart(pageStart);

            if (nextPageStart >= currentDocument.byteLength)
            {
                pageIndexReady = true;
                return true;
            }

            if (
                nextPageStart <= pageStart ||
                !addPageStart(nextPageStart)
            ) {
                clearPageIndex();
                return false;
            }

            pageStart = nextPageStart;
        }

        pageIndexReady = true;
        return true;
    }

    uint32_t findPageStartWithoutIndex(uint16_t pageIndex)
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

    uint16_t countPagesWithoutIndex()
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

    uint32_t getPageStart(uint16_t pageIndex)
    {
        if (pageIndexReady && pageIndex < pageStartCount)
        {
            return pageStartOffsets[pageIndex];
        }

        return findPageStartWithoutIndex(pageIndex);
    }

    uint16_t getPageCount()
    {
        if (pageIndexReady)
        {
            return pageStartCount;
        }

        return countPagesWithoutIndex();
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

void ReaderPage::open(
    const CachedBook* book,
    const ReaderDocument& document,
    uint16_t savedPage
)
{
    currentBook = book;
    currentDocument = document;
    currentPage = 0;
    selectedEmptyOption = 0;
    readerNeedsFullRefresh = true;
    partialTurnsSinceFullRefresh = 0;

    if (hasOpenDocument() && !buildPageIndex())
    {
        Serial.println(F("Reader page index unavailable"));
    }

    const uint16_t pageCount = getPageCount();

    if (pageCount > 0)
    {
        const uint16_t lastPage = pageCount - 1;
        currentPage = savedPage > lastPage ? lastPage : savedPage;
    }

    if (currentBook != nullptr)
    {
        saveCachedBookPage(*currentBook, currentPage);
    }
}

bool moveReaderPreviousPage()
{
    if (currentBook == nullptr || currentPage == 0)
    {
        return false;
    }

    currentPage--;
    saveCachedBookPage(*currentBook, currentPage);
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
    saveCachedBookPage(*currentBook, currentPage);
    return true;
}

bool ReaderPage::handleInput(const InputState& input)
{
    if (!input.upPressed && !input.downPressed)
    {
        return false;
    }

    if (!hasOpenDocument())
    {
        const uint8_t previousIndex = selectedEmptyOption;

        if (input.upPressed && !input.downPressed && selectedEmptyOption > 0)
        {
            selectedEmptyOption--;
        }
        else if (
            input.downPressed && !input.upPressed &&
            selectedEmptyOption + 1 < READER_EMPTY_OPTION_COUNT
        ) {
            selectedEmptyOption++;
        }

        if (selectedEmptyOption != previousIndex)
        {
            draw(85);
        }

        return true;
    }

    bool pageChanged = false;

    if (input.upPressed && !input.downPressed)
    {
        pageChanged = moveReaderPreviousPage();
    }
    else if (input.downPressed && !input.upPressed)
    {
        pageChanged = moveReaderNextPage();
    }

    if (pageChanged)
    {
        draw(85);
    }

    return true;
}

NavigationRequest ReaderPage::select()
{
    if (hasOpenDocument() || selectedEmptyOption >= READER_EMPTY_OPTION_COUNT)
    {
        return noNavigation();
    }

    return READER_EMPTY_OPTIONS[selectedEmptyOption];
}

void ReaderPage::onEnter()
{
    readerNeedsFullRefresh = true;
}

void ReaderPage::draw(uint8_t batteryPercent)
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
            const char* options[MAX_NAVIGATION_OPTIONS];
            getNavigationOptionLabels(
                READER_EMPTY_OPTIONS,
                options,
                READER_EMPTY_OPTION_COUNT
            );

            if (useFullRefresh)
            {
                drawHeader("READER", batteryPercent);
            }

            drawMessage(
                "Nothing in Progress",
                nullptr,
                options,
                READER_EMPTY_OPTION_COUNT,
                selectedEmptyOption
            );
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

#include "screens/Reader.h"

#include <stdio.h>

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"
#include "components/PageContent.h"
#include "navigation/PageRegistry.h"

namespace
{
    constexpr uint8_t READER_VERTICAL_PADDING = 8;
    constexpr uint8_t READER_LINE_HEIGHT = 18;
    constexpr uint8_t READER_TEXT_BASELINE = 13;
    constexpr uint8_t MAX_LINE_LENGTH = 48;
    constexpr uint8_t PARTIAL_TURNS_BEFORE_FULL_REFRESH = 12;
    constexpr uint8_t PAGE_TURNS_BEFORE_PROGRESS_FLUSH = 5;
}

bool ReaderPage::hasOpenDocument() const
{
    return
        currentDocument.isOpen();
}

uint8_t ReaderPage::getMaximumCharactersPerLine() const
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

uint8_t ReaderPage::getLinesPerPage() const
    {
        const int contentHeight =
            display.height() -
            Theme::HEADER_HEIGHT -
            Theme::FOOTER_HEIGHT -
            (READER_VERTICAL_PADDING * 2);

        return contentHeight / READER_LINE_HEIGHT;
    }

void ReaderPage::drawCurrentTextPage() const
    {
        const int contentTop =
            Theme::HEADER_HEIGHT + READER_VERTICAL_PADDING;
        const int contentBottom =
            display.height() -
            Theme::FOOTER_HEIGHT - READER_VERTICAL_PADDING;

        char line[MAX_LINE_LENGTH];
        uint32_t position = paginator.pageStart(currentPage);

        display.setFont(Theme::BODY_FONT);
        display.setTextColor(Theme::TEXT_COLOR);

        for (
            int baseline = contentTop + READER_TEXT_BASELINE;
            baseline < contentBottom &&
                position < currentDocument.length();
            baseline += READER_LINE_HEIGHT
        ) {
            position = paginator.nextLine(
                position,
                line,
                sizeof(line)
            );

            if (line[0] != '\0')
            {
                display.setCursor(Theme::PAGE_MARGIN, baseline);
                display.print(line);
            }
        }
    }

bool ReaderPage::open(
    const CachedBook* book,
    uint16_t savedPage
)
{
    flushCachedBookProgress();
    paginator.clear();
    currentDocument.close();
    currentBook = book;
    currentPage = 0;
    selectedEmptyOption = 0;
    readerNeedsFullRefresh = true;
    partialTurnsSinceFullRefresh = 0;
    pageTurnsSinceProgressFlush = 0;

    if (currentBook == nullptr || !currentDocument.open(*currentBook))
    {
        currentBook = nullptr;
        return false;
    }

    if (!paginator.configure(
        currentDocument,
        getMaximumCharactersPerLine(),
        getLinesPerPage()
    ))
    {
        Serial.println(F("Reader page index unavailable"));
    }

    const uint16_t pageCount = paginator.pageCount();

    if (pageCount > 0)
    {
        const uint16_t lastPage = pageCount - 1;
        currentPage = savedPage > lastPage ? lastPage : savedPage;
    }

    if (currentBook != nullptr)
    {
        saveCachedBookPage(*currentBook, currentPage);
        setLastOpenedCachedBook(*currentBook);
        flushCachedBookProgress();
    }

    return true;
}

bool ReaderPage::movePreviousPage()
{
    if (currentBook == nullptr || currentPage == 0)
    {
        return false;
    }

    currentPage--;
    recordPageChange();
    return true;
}

bool ReaderPage::moveNextPage()
{
    const uint16_t pageCount = paginator.pageCount();

    if (pageCount == 0 || currentPage + 1 >= pageCount)
    {
        return false;
    }

    currentPage++;
    recordPageChange();
    return true;
}

void ReaderPage::recordPageChange()
{
    saveCachedBookPage(*currentBook, currentPage);
    pageTurnsSinceProgressFlush++;

    if (
        pageTurnsSinceProgressFlush >=
            PAGE_TURNS_BEFORE_PROGRESS_FLUSH &&
        flushCachedBookProgress()
    ) {
        pageTurnsSinceProgressFlush = 0;
    }
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
        pageChanged = movePreviousPage();
    }
    else if (input.downPressed && !input.upPressed)
    {
        pageChanged = moveNextPage();
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

void ReaderPage::onStartup()
{
    const CachedBook* lastOpenedBook = getLastOpenedCachedBook();

    if (lastOpenedBook != nullptr)
    {
        if (!open(lastOpenedBook, getCachedBookPage(*lastOpenedBook)))
        {
            Serial.print(F("Could not reopen current book: "));
            Serial.println(lastOpenedBook->id);
        }
    }
}

void ReaderPage::onEnter()
{
    readerNeedsFullRefresh = true;
}

void ReaderPage::onExit()
{
    if (flushCachedBookProgress())
    {
        pageTurnsSinceProgressFlush = 0;
    }
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
            Theme::HEADER_HEIGHT,
            display.width(),
            display.height() - Theme::HEADER_HEIGHT
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
            clearPageContent();
            display.fillRect(
                0,
                display.height() - Theme::FOOTER_HEIGHT + 1,
                display.width(),
                Theme::FOOTER_HEIGHT - 1,
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
            paginator.pageCount()
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

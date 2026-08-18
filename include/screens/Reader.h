#pragma once

#include <Arduino.h>

#include "books/BookCache.h"
#include "books/TextPaginator.h"
#include "navigation/Page.h"

class ReaderPage : public Page
{
public:
    ReaderPage() = default;

    ReaderPage(const ReaderPage&) = delete;
    ReaderPage& operator=(const ReaderPage&) = delete;

    bool open(
        const CachedBook* book,
        uint16_t savedPage
    );

    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;
    void onEnter() override;
    void onExit() override;

private:
    const CachedBook* currentBook = nullptr;
    ReaderDocument currentDocument = {};
    uint16_t currentPage = 0;
    uint8_t selectedEmptyOption = 0;
    bool readerNeedsFullRefresh = true;
    uint8_t partialTurnsSinceFullRefresh = 0;
    uint8_t pageTurnsSinceProgressFlush = 0;
    TextPaginator paginator;

    bool hasOpenDocument() const;
    uint8_t getMaximumCharactersPerLine() const;
    uint8_t getLinesPerPage() const;
    void drawCurrentTextPage() const;
    bool movePreviousPage();
    bool moveNextPage();
    void recordPageChange();
};

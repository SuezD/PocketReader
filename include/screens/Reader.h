#pragma once

#include <Arduino.h>

#include "books/BookCache.h"
#include "navigation/Page.h"

class ReaderPage : public Page
{
public:
    ReaderPage() = default;
    ~ReaderPage() override;

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

private:
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

    bool hasOpenDocument() const;
    char readCharacter(uint32_t position) const;
    uint8_t getMaximumCharactersPerLine() const;
    uint32_t readNextLine(uint32_t source, char* output) const;
    uint8_t getLinesPerPage() const;
    uint32_t getNextPageStart(uint32_t pageStart) const;
    void clearPageIndex();
    bool addPageStart(uint32_t pageStart);
    bool buildPageIndex();
    uint32_t findPageStartWithoutIndex(uint16_t pageIndex) const;
    uint16_t countPagesWithoutIndex() const;
    uint32_t getPageStart(uint16_t pageIndex) const;
    uint16_t getPageCount() const;
    void drawCurrentTextPage() const;
    bool movePreviousPage();
    bool moveNextPage();
};

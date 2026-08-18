#pragma once

#include <stddef.h>
#include <stdint.h>

#include "books/TextDocument.h"

class TextPaginator
{
public:
    TextPaginator() = default;
    ~TextPaginator();

    TextPaginator(const TextPaginator&) = delete;
    TextPaginator& operator=(const TextPaginator&) = delete;

    bool configure(
        const TextDocument& document,
        uint8_t maximumCharactersPerLine,
        uint8_t linesPerPage
    );
    void clear();

    uint16_t pageCount() const;
    uint32_t pageStart(uint16_t pageIndex) const;
    uint32_t nextLine(
        uint32_t source,
        char* output,
        size_t outputSize
    ) const;

private:
    static constexpr uint16_t INITIAL_INDEX_CAPACITY = 16;

    const TextDocument* document = nullptr;
    uint8_t maximumCharactersPerLine = 0;
    uint8_t linesPerPage = 0;
    uint32_t* pageStartOffsets = nullptr;
    uint16_t pageStartCount = 0;
    uint16_t pageStartCapacity = 0;
    bool pageIndexReady = false;

    bool addPageStart(uint32_t pageStart);
    bool buildPageIndex();
    uint32_t nextPageStart(uint32_t pageStart) const;
    uint32_t findPageStartWithoutIndex(uint16_t pageIndex) const;
    uint16_t countPagesWithoutIndex() const;
};

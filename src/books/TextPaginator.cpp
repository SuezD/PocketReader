#include "books/TextPaginator.h"

#include <algorithm>
#include <stdlib.h>

namespace
{
    bool isInlineWhitespace(char character)
    {
        return character == ' ' || character == '\t';
    }
}

TextPaginator::~TextPaginator()
{
    clear();
}

bool TextPaginator::configure(
    const TextDocument& newDocument,
    uint8_t newMaximumCharactersPerLine,
    uint8_t newLinesPerPage
)
{
    clear();

    if (
        !newDocument.isOpen() ||
        newMaximumCharactersPerLine == 0 ||
        newLinesPerPage == 0
    ) {
        return false;
    }

    document = &newDocument;
    maximumCharactersPerLine = newMaximumCharactersPerLine;
    linesPerPage = newLinesPerPage;
    return buildPageIndex();
}

void TextPaginator::clear()
{
    free(pageStartOffsets);
    pageStartOffsets = nullptr;
    pageStartCount = 0;
    pageStartCapacity = 0;
    pageIndexReady = false;
    document = nullptr;
    maximumCharactersPerLine = 0;
    linesPerPage = 0;
}

uint32_t TextPaginator::nextLine(
    uint32_t source,
    char* output,
    size_t outputSize
) const {
    if (output == nullptr || outputSize == 0)
    {
        return source;
    }

    output[0] = '\0';

    if (document == nullptr || source >= document->length())
    {
        return source;
    }

    if (document->readCharacter(source) == '\n')
    {
        return source + 1;
    }

    while (
        source < document->length() &&
        isInlineWhitespace(document->readCharacter(source))
    ) {
        source++;
    }

    const size_t outputLimit =
        std::min(
            static_cast<size_t>(maximumCharactersPerLine),
            outputSize - 1
        );
    size_t outputLength = 0;
    uint32_t position = source;

    while (position < document->length())
    {
        const char character = document->readCharacter(position);

        if (character == '\n')
        {
            output[outputLength] = '\0';
            return position + 1;
        }

        if (isInlineWhitespace(character))
        {
            position++;
            continue;
        }

        const uint32_t wordStart = position;
        uint32_t wordLength = 0;

        while (
            position < document->length() &&
            document->readCharacter(position) != '\n' &&
            !isInlineWhitespace(document->readCharacter(position))
        ) {
            wordLength++;
            position++;
        }

        const size_t separatorLength = outputLength == 0 ? 0 : 1;

        if (
            outputLength > 0 &&
            outputLength + separatorLength + wordLength > outputLimit
        ) {
            output[outputLength] = '\0';
            return wordStart;
        }

        if (outputLength == 0 && wordLength > outputLimit)
        {
            for (size_t index = 0; index < outputLimit; index++)
            {
                output[index] = document->readCharacter(wordStart + index);
            }

            output[outputLimit] = '\0';
            return wordStart + outputLimit;
        }

        if (separatorLength > 0)
        {
            output[outputLength++] = ' ';
        }

        for (uint32_t index = 0; index < wordLength; index++)
        {
            output[outputLength++] =
                document->readCharacter(wordStart + index);
        }
    }

    output[outputLength] = '\0';
    return position;
}

uint32_t TextPaginator::nextPageStart(uint32_t pageStart) const
{
    char line[64];
    uint32_t position = pageStart;

    for (
        uint8_t lineIndex = 0;
        lineIndex < linesPerPage &&
            document != nullptr &&
            position < document->length();
        lineIndex++
    ) {
        position = nextLine(position, line, sizeof(line));
    }

    return position;
}

bool TextPaginator::addPageStart(uint32_t newPageStart)
{
    if (pageStartCount == UINT16_MAX)
    {
        return false;
    }

    if (pageStartCount == pageStartCapacity)
    {
        uint32_t expandedCapacity =
            pageStartCapacity == 0
                ? INITIAL_INDEX_CAPACITY
                : static_cast<uint32_t>(pageStartCapacity) * 2;

        if (expandedCapacity > UINT16_MAX)
        {
            expandedCapacity = UINT16_MAX;
        }

        void* expandedOffsets = realloc(
            pageStartOffsets,
            expandedCapacity * sizeof(*pageStartOffsets)
        );

        if (expandedOffsets == nullptr)
        {
            return false;
        }

        pageStartOffsets = static_cast<uint32_t*>(expandedOffsets);
        pageStartCapacity = expandedCapacity;
    }

    pageStartOffsets[pageStartCount++] = newPageStart;
    return true;
}

bool TextPaginator::buildPageIndex()
{
    if (document == nullptr || !addPageStart(0))
    {
        return false;
    }

    uint32_t currentPageStart = 0;

    while (currentPageStart < document->length())
    {
        const uint32_t followingPageStart =
            nextPageStart(currentPageStart);

        if (followingPageStart >= document->length())
        {
            pageIndexReady = true;
            return true;
        }

        if (
            followingPageStart <= currentPageStart ||
            !addPageStart(followingPageStart)
        ) {
            free(pageStartOffsets);
            pageStartOffsets = nullptr;
            pageStartCount = 0;
            pageStartCapacity = 0;
            return false;
        }

        currentPageStart = followingPageStart;
    }

    pageIndexReady = true;
    return true;
}

uint32_t TextPaginator::findPageStartWithoutIndex(
    uint16_t pageIndex
) const {
    uint32_t result = 0;

    for (
        uint16_t index = 0;
        index < pageIndex &&
            document != nullptr &&
            result < document->length();
        index++
    ) {
        result = nextPageStart(result);
    }

    return result;
}

uint16_t TextPaginator::countPagesWithoutIndex() const
{
    if (document == nullptr || !document->isOpen())
    {
        return 0;
    }

    uint16_t count = 1;
    uint32_t currentPageStart = 0;

    while (true)
    {
        const uint32_t followingPageStart =
            nextPageStart(currentPageStart);

        if (followingPageStart >= document->length())
        {
            return count;
        }

        if (count == UINT16_MAX || followingPageStart <= currentPageStart)
        {
            return count;
        }

        count++;
        currentPageStart = followingPageStart;
    }
}

uint16_t TextPaginator::pageCount() const
{
    return pageIndexReady ? pageStartCount : countPagesWithoutIndex();
}

uint32_t TextPaginator::pageStart(uint16_t pageIndex) const
{
    if (pageIndexReady && pageIndex < pageStartCount)
    {
        return pageStartOffsets[pageIndex];
    }

    return findPageStartWithoutIndex(pageIndex);
}

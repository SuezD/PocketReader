#include "books/BookCache.h"

#include <string.h>

namespace
{
    const CachedBook BOOKS[] = {
        { "book-1", "The Hobbit" }
    };

    const char* const BOOK_TITLES[] = {
        BOOKS[0].title
    };

    const char* const BOOK_TEXTS[] = {
        "Then the hobbit slipped on his ring, "
    };

    constexpr uint8_t BOOK_COUNT =
        sizeof(BOOKS) / sizeof(BOOKS[0]);
}

uint8_t getCachedBookCount()
{
    return BOOK_COUNT;
}

const char* const* getCachedBookTitles()
{
    return BOOK_TITLES;
}

const CachedBook& getCachedBook(uint8_t index)
{
    return BOOKS[index < BOOK_COUNT ? index : 0];
}

const char* readCachedBookText(const CachedBook& book)
{
    for (uint8_t index = 0; index < BOOK_COUNT; index++)
    {
        if (strcmp(book.id, BOOKS[index].id) == 0)
        {
            return BOOK_TEXTS[index];
        }
    }

    return nullptr;
}

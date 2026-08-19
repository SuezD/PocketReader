#pragma once

#include "books/BookCache.h"
#include "books/SelectedCachedBook.h"
#include "components/SelectList.h"
#include "navigation/Page.h"
#include "screens/Reader.h"

class MyBooksPage : public Page
{
public:
    explicit MyBooksPage(SelectedCachedBook& selectedBook);

    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    static constexpr uint8_t MAX_BOOK_ITEMS = 16;
    SelectedCachedBook& selectedBook;
    SelectListState listState = {};
    uint8_t selectedEmptyOption = 0;

    uint8_t getItems(const char** items) const;
    void getFooterText(
        char* leftText,
        size_t leftTextSize,
        char* rightText,
        size_t rightTextSize
    ) const;
};

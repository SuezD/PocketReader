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
    SelectedCachedBook& selectedBook;
    SelectListState listState = {};
    uint8_t selectedEmptyOption = 0;
};

#pragma once

#include "books/BookCache.h"
#include "components/SelectList.h"
#include "navigation/Page.h"
#include "screens/Reader.h"

class MyBooksPage : public Page
{
public:
    explicit MyBooksPage(ReaderPage& readerPage);

    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    ReaderPage& readerPage;
    SelectListState listState = {};
};

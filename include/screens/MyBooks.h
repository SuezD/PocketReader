#pragma once

#include "books/BookCache.h"
#include "components/SelectList.h"
#include "navigation/Page.h"
#include "screens/Reader.h"

class MyBooksPage : public Page
{
public:
    explicit MyBooksPage(ReaderPage& readerPage);

    void onEnter() override;
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    enum class State : uint8_t
    {
        Catalogue,
        BookActions,
        DeleteConfirmation
    };

    static constexpr uint8_t MAX_BOOK_ITEMS = 16;
    ReaderPage& readerPage;
    State state = State::Catalogue;
    SelectListState listState = {};
    uint8_t selectedEmptyOption = 0;
    uint8_t optionSelection = 0;
    uint8_t batteryPercent = 0;
    char selectedBookId[32] = {};
    char selectedBookTitle[64] = {};
    char status[64] = {};

    uint8_t getItems(const char** items) const;
    void getFooterText(
        char* leftText,
        size_t leftTextSize,
        char* rightText,
        size_t rightTextSize
    ) const;
    void selectBook(const CachedBook& book);
    void setStatus(const char* nextStatus);
    void drawBody();
    void drawCurrentContent();
    void redrawCurrentSelection(uint8_t previousIndex);
    void enterState(State nextState);
};

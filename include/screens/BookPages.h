#pragma once

#include "books/SelectedCachedBook.h"
#include "components/CenteredOptionPage.h"
#include "navigation/Page.h"
#include "screens/Reader.h"

class BookActionsPage : public Page
{
public:
    BookActionsPage(SelectedCachedBook& selectedBook, ReaderPage& readerPage);
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    SelectedCachedBook& selectedBook;
    ReaderPage& readerPage;
    CenteredOptionPage optionsPage;
};

class DeleteBookPage : public Page
{
public:
    DeleteBookPage(SelectedCachedBook& selectedBook, ReaderPage& readerPage);
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    SelectedCachedBook& selectedBook;
    ReaderPage& readerPage;
    CenteredOptionPage optionsPage;
};

#pragma once

#include "navigation/Page.h"
#include "books/BookSync.h"

class AddBooksPage : public Page
{
public:
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    BookSyncResult syncResult = BookSyncResult::NotConfigured;
};

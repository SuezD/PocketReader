#pragma once

#include <Arduino.h>

#include "books/BookCache.h"
#include "navigation/Page.h"

class ReaderPage : public Page
{
public:
    void open(
        const CachedBook* book,
        const ReaderDocument& document,
        uint16_t savedPage
    );

    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;
    void onEnter() override;
};

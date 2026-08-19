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
    enum class Action : uint8_t
    {
        Retry,
        WifiSettings
    };

    static constexpr uint8_t MAX_ACTION_COUNT = 1;

    BookSyncResult syncResult = BookSyncResult::NotConfigured;
    uint8_t batteryPercent = 0;

    uint8_t getActions(Action* actions, const char** labels) const;
    void refresh();
    void fetchAndRenderResult();
    void drawLoadingContent();
    void drawResultContent();
};

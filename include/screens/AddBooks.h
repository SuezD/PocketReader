#pragma once

#include "navigation/Page.h"
#include "books/BookSync.h"
#include "components/SelectList.h"
#include "screens/Reader.h"

class AddBooksPage : public Page
{
public:
    explicit AddBooksPage(ReaderPage& readerPage);
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;

private:
    enum class State : uint8_t
    {
        Fetching,
        Catalogue,
        Error,
        Downloading,
        DownloadComplete
    };

    enum class Action : uint8_t
    {
        Retry,
        WifiSettings,
        Back
    };

    static constexpr uint8_t MAX_ACTION_COUNT = 2;
    static constexpr uint8_t MAX_CATALOG_BOOKS = 16;
    static constexpr uint8_t MAX_CATALOG_ITEMS = MAX_CATALOG_BOOKS;
    static constexpr uint8_t DOWNLOAD_COMPLETE_OPTION_COUNT = 3;

    ReaderPage& readerPage;
    State state = State::Fetching;
    BookSyncResult syncResult = BookSyncResult::NotConfigured;
    uint8_t batteryPercent = 0;
    SelectListState listState = {};
    uint8_t selectedActionIndex = 0;
    String downloadStatus;
    String completedBookId;
    String completedBookTitle;
    uint8_t selectedCompleteOption = 0;

    uint8_t getActions(Action* actions, const char** labels) const;
    void refresh();
    void fetchAndRenderResult();
    void drawLoadingContent();
    void drawResultContent();
    void drawDownloadCompleteContent();
    uint8_t getAvailableBookCount() const;
    const RemoteBook* getAvailableBook(uint8_t availableIndex) const;
    void getBookTitles(const char** titles) const;
    void downloadSelectedBook();
};

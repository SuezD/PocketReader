#pragma once

#include "navigation/Page.h"
#include "books/BookSync.h"
#include "components/SelectList.h"
#include "screens/Reader.h"

class AddBooksPage : public Page
{
public:
    explicit AddBooksPage(ReaderPage& readerPage);
    void onEnter() override;
    void onExit() override;
    void draw(uint8_t batteryPercent) override;
    bool handleInput(const InputState& input) override;
    NavigationRequest select() override;
    bool handleConnectivityStateChange(
        uint8_t batteryPercent,
        bool wifiStateChanged,
        bool portalStateChanged
    ) override;

private:
    enum class State : uint8_t
    {
        Fetching,
        Catalogue,
        Error,
        ServerSetupInstructions,
        Downloading,
        DownloadFailed,
        DownloadComplete
    };

    enum class FailureAction : uint8_t
    {
        Retry,
        WifiSettings,
        MyBooks,
        Back
    };

    enum class Action : uint8_t
    {
        Retry,
        WifiSettings,
        ConfigureServer,
        Back
    };

    static constexpr uint8_t MAX_ACTION_COUNT = 3;
    static constexpr uint8_t MAX_CATALOG_BOOKS = 16;
    static constexpr uint8_t MAX_CATALOG_ITEMS = MAX_CATALOG_BOOKS;
    static constexpr uint8_t DOWNLOAD_COMPLETE_OPTION_COUNT = 3;
    static constexpr uint8_t MAX_FAILURE_OPTION_COUNT = 2;

    ReaderPage& readerPage;
    State state = State::Fetching;
    BookSyncResult syncResult = BookSyncResult::NotConfigured;
    uint8_t batteryPercent = 0;
    SelectListState listState = {};
    uint8_t selectedActionIndex = 0;
    String downloadStatus;
    String completedBookId;
    String completedBookTitle;
    String downloadFailureMessage;
    FailureAction failurePrimaryAction = FailureAction::Retry;
    uint8_t selectedFailureOption = 0;
    uint8_t selectedCompleteOption = 0;
    bool fetchPending = false;
    bool catalogueFetchAttempted = false;
    State stateBeforeServerSetup = State::Error;
    uint32_t catalogueServerRevision = 0;
    uint32_t setupServerRevision = 0;

    uint8_t getActions(Action* actions, const char** labels) const;
    void refresh();
    void openServerSetup();
    String getServerSetupInstructions() const;
    void fetchAndRenderResult();
    void drawBody();
    void drawCurrentContent();
    void showDownloadFailure(
        const char* message,
        const char* bookId,
        const char* bookTitle,
        FailureAction primaryAction
    );
    uint8_t getFailureOptions(
        FailureAction* actions,
        const char** labels
    ) const;
    uint8_t getAvailableBookCount() const;
    const RemoteBook* getAvailableBook(uint8_t availableIndex) const;
    void getBookTitles(const char** titles) const;
    void getFooterText(
        char* leftText,
        size_t leftTextSize,
        char* rightText,
        size_t rightTextSize
    ) const;
    void drawStorageFooter() const;
    void redrawStorageFooter() const;
    void downloadSelectedBook();
};

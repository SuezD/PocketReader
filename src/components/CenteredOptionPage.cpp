#include "CenteredOptionPage.h"

#include "Display.h"
#include "Theme.h"
#include "CenteredMessage.h"
#include "Footer.h"
#include "Header.h"
#include "Selection.h"

void CenteredOptionPage::draw(
    const char* heading,
    const char* firstLine,
    const char* secondLine,
    const char* const options[],
    uint8_t optionCount,
    uint8_t batteryPercent
) {
    selection = 0;
    display.setFullWindow();
    display.firstPage();
    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader(heading, batteryPercent);
        drawMessage(
            firstLine,
            secondLine,
            options,
            optionCount,
            selection
        );
        drawFooter();
    }
    while (display.nextPage());
}

bool CenteredOptionPage::handleInput(
    const InputState& input,
    const char* firstLine,
    const char* secondLine,
    const char* const options[],
    uint8_t optionCount
) {
    const uint8_t previousSelection = selection;
    if (!moveSelection(input, selection, optionCount))
    {
        return input.upPressed || input.downPressed;
    }
    redrawMessageSelection(
        firstLine,
        secondLine,
        options,
        optionCount,
        previousSelection,
        selection
    );
    return true;
}

uint8_t CenteredOptionPage::selectedIndex() const
{
    return selection;
}

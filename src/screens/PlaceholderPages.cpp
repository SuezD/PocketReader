#include "screens/PlaceholderPages.h"

#include "Display.h"
#include "Theme.h"
#include "components/CenteredMessage.h"
#include "components/Footer.h"
#include "components/Header.h"

PlaceholderPage::PlaceholderPage(
    const char* header,
    const char* message
) : header(header), message(message)
{
}

void PlaceholderPage::draw(uint8_t batteryPercent)
{
    display.setFullWindow();
    display.firstPage();

    do
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        drawHeader(header, batteryPercent);
        drawMessage(message, "Coming soon");
        drawFooter();
    }
    while (display.nextPage());
}

bool PlaceholderPage::handleInput(const InputState& input)
{
    return input.upPressed || input.downPressed;
}

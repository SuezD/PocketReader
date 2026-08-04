#include "Startup.h"

#include "Display.h"
#include "Theme.h"
#include "components/Header.h"
#include "components/Footer.h"
#include "components/CenteredMessage.h"

namespace
{
    void drawStartupContents(bool isReady)
    {
        display.fillScreen(Theme::BACKGROUND_COLOR);
        display.setTextColor(Theme::TEXT_COLOR);

        display.setFont(Theme::HEADER_FONT);
        display.setCursor(Theme::PAGE_MARGIN, 80);

        display.setFont(Theme::BODY_FONT);
        display.setCursor(Theme::PAGE_MARGIN, 140);
        
        drawHeader("POCKET READER", 85);

        if (isReady)
        {
            drawMessage(
                "Press any button",
                "to continue"
            );
        }
        else
        {
            drawMessage("Starting...");
        }

        drawFooter();
    }
}

void drawStartupScreen(bool isReady)
{
    display.setFullWindow();
    display.firstPage();

    do
    {
        drawStartupContents(isReady);
    }
    while (display.nextPage());
}

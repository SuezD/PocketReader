#include "PageContent.h"

#include "Display.h"
#include "Theme.h"

namespace
{
    int getFooterTop()
    {
        return display.height() - Theme::FOOTER_HEIGHT;
    }
}

void setPageContentPartialWindow()
{
    const int footerTop = getFooterTop();

    // Include both dividers because the rotated e-paper driver may expand
    // partial windows to byte-aligned native display coordinates.
    display.setPartialWindow(
        0,
        Theme::HEADER_HEIGHT,
        display.width(),
        footerTop - Theme::HEADER_HEIGHT + 1
    );
}

void clearPageContent()
{
    const int contentTop = Theme::HEADER_HEIGHT + 1;
    const int footerTop = getFooterTop();

    display.fillRect(
        0,
        contentTop,
        display.width(),
        footerTop - contentTop,
        Theme::BACKGROUND_COLOR
    );
    display.drawLine(
        0,
        Theme::HEADER_HEIGHT,
        display.width() - 1,
        Theme::HEADER_HEIGHT,
        Theme::BORDER_COLOR
    );
    display.drawLine(
        0,
        footerTop,
        display.width() - 1,
        footerTop,
        Theme::BORDER_COLOR
    );
}

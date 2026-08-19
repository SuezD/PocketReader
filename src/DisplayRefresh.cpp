#include "DisplayRefresh.h"

#include "Display.h"

namespace
{
    constexpr uint32_t FULL_REFRESH_EQUIVALENTS = 10;
    constexpr uint32_t MINIMUM_PARTIAL_COST_DIVISOR = 20;

    uint32_t partialRefreshCost = 0;

    uint32_t getDisplayArea()
    {
        return static_cast<uint32_t>(display.width()) * display.height();
    }
}

void setFullRefreshWindow()
{
    partialRefreshCost = 0;
    display.setFullWindow();
}

void setPartialRefreshWindow(
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height
) {
    const uint32_t displayArea = getDisplayArea();
    const uint32_t updateArea = static_cast<uint32_t>(width) * height;
    const uint32_t minimumCost =
        displayArea / MINIMUM_PARTIAL_COST_DIVISOR;
    const uint32_t cost = max(updateArea, minimumCost);
    const uint32_t maximumCost = displayArea * FULL_REFRESH_EQUIVALENTS;

    partialRefreshCost = min(partialRefreshCost + cost, maximumCost);
    display.setPartialWindow(x, y, width, height);
}

void setPageRefreshWindow()
{
    if (isFullRefreshDue())
    {
        setFullRefreshWindow();
        return;
    }

    setPartialRefreshWindow(0, 0, display.width(), display.height());
}

bool isFullRefreshDue()
{
    return partialRefreshCost >= getDisplayArea() * FULL_REFRESH_EQUIVALENTS;
}

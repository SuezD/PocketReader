// Display.cpp
#include "Display.h"
#include "BoardConfig.h"

DisplayType display(
    EpaperDriver(
        BoardConfig::EPD_CS,
        BoardConfig::EPD_DC,
        BoardConfig::EPD_RST,
        BoardConfig::EPD_BUSY
    )
);

void initDisplay()
{
    display.init(115200, true, 2, false);
    display.setRotation(3); // 300 × 400 portrait
}

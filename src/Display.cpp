// Display.cpp
#include "Display.h"
#include "BoardConfig.h"

DisplayType display(
    EpaperDriver(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

void initDisplay()
{
    display.init(0, true, 2, false);
    display.setRotation(3); // 300 × 400 portrait
}

// Display.cpp
#include "Display.h"
#include "BoardConfig.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <SPI.h>
#endif

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
#if defined(ARDUINO_ARCH_ESP32)
    SPI.begin(
        BoardConfig::EPD_SCK,
        -1,
        BoardConfig::EPD_MOSI,
        BoardConfig::EPD_CS
    );
#endif

    display.init(115200, true, 2, false);
    display.setRotation(3); // 300 × 400 portrait
}

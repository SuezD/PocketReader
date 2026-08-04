#include <Arduino.h>

#include "Display.h"
#include "components/Header.h"
#include "components/Footer.h"

void setup()
{
    initDisplay();
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(GxEPD_WHITE);
        drawHeader("POCKET READER", 85); // hardcoded battery percentage for now
        drawFooter("Left Footer Text", "Right Footer Text");
    } while (display.nextPage());

    display.hibernate();
}

void loop()
{
}

// Standalone 300 x 400 portrait splash-screen design test.
// Board: Seeed Studio XIAO ESP32-S3
// Display: Waveshare 4.2" monochrome Rev 2.2 (GDEY042T81)

#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <SPI.h>
#endif
#include <GxEPD2_BW.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>

namespace Hardware
{
    constexpr int EPD_SCK  = 7; // D8
    constexpr int EPD_MOSI = 9; // D10
    constexpr int EPD_CS   = 3; // D2
    constexpr int EPD_DC   = 4; // D3
    constexpr int EPD_RST  = 5; // D4
    constexpr int EPD_BUSY = 6; // D5
}

using EpaperDriver = GxEPD2_420_GDEY042T81;
using Display = GxEPD2_BW<EpaperDriver, EpaperDriver::HEIGHT>;

Display display(EpaperDriver(
    Hardware::EPD_CS,
    Hardware::EPD_DC,
    Hardware::EPD_RST,
    Hardware::EPD_BUSY
));

static void printCentered(const char* text, int16_t baseline)
{
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;
    display.getTextBounds(text, 0, baseline, &x1, &y1, &width, &height);
    display.setCursor((display.width() - width) / 2 - x1, baseline);
    display.print(text);
}

static void drawWifiIcon(int16_t centerX, int16_t top)
{
    // Three deliberately pixel-drawn arcs reproduce the compact iOS-style mark.
    display.drawPixel(centerX, top + 10, GxEPD_BLACK);
    display.drawLine(centerX - 2, top + 7, centerX, top + 6, GxEPD_BLACK);
    display.drawLine(centerX, top + 6, centerX + 2, top + 7, GxEPD_BLACK);
    display.drawLine(centerX - 5, top + 4, centerX - 2, top + 2, GxEPD_BLACK);
    display.drawLine(centerX - 2, top + 2, centerX + 2, top + 2, GxEPD_BLACK);
    display.drawLine(centerX + 2, top + 2, centerX + 5, top + 4, GxEPD_BLACK);
    display.drawLine(centerX - 7, top + 2, centerX - 4, top, GxEPD_BLACK);
    display.drawLine(centerX - 4, top, centerX + 4, top, GxEPD_BLACK);
    display.drawLine(centerX + 4, top, centerX + 7, top + 2, GxEPD_BLACK);
}

static void drawBatteryIcon(int16_t x, int16_t y)
{
    display.drawRoundRect(x, y, 20, 9, 2, GxEPD_BLACK);
    display.fillRect(x + 2, y + 2, 16, 5, GxEPD_BLACK);
    display.fillRect(x + 20, y + 3, 2, 3, GxEPD_BLACK);
}

static void drawBookIcon(int16_t centerX, int16_t top)
{
    constexpr int16_t halfWidth = 28;
    constexpr int16_t height = 43;
    const int16_t left = centerX - halfWidth;
    const int16_t right = centerX + halfWidth;
    const int16_t bottom = top + height;

    // Open-book silhouette with a shallow curved page crown.
    display.drawLine(left, top + 4, left, bottom, GxEPD_BLACK);
    display.drawLine(right, top + 4, right, bottom, GxEPD_BLACK);
    display.drawLine(left, bottom, centerX, bottom - 5, GxEPD_BLACK);
    display.drawLine(centerX, bottom - 5, right, bottom, GxEPD_BLACK);
    display.drawLine(centerX, top + 5, centerX, bottom - 5, GxEPD_BLACK);

    display.drawLine(left, top + 4, left + 7, top + 1, GxEPD_BLACK);
    display.drawLine(left + 7, top + 1, left + 15, top + 1, GxEPD_BLACK);
    display.drawLine(left + 15, top + 1, centerX, top + 5, GxEPD_BLACK);
    display.drawLine(centerX, top + 5, centerX + 7, top + 1, GxEPD_BLACK);
    display.drawLine(centerX + 7, top + 1, centerX + 15, top + 1, GxEPD_BLACK);
    display.drawLine(centerX + 15, top + 1, right, top + 4, GxEPD_BLACK);

    // A second pass gives the reference icon its slightly heavier ink weight.
    display.drawLine(left + 1, top + 5, left + 1, bottom - 1, GxEPD_BLACK);
    display.drawLine(right - 1, top + 5, right - 1, bottom - 1, GxEPD_BLACK);
}

static void drawSplash()
{
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    display.setFont(nullptr);
    display.setTextSize(1);
    display.setCursor(16, 27);
    display.print(F("10:42"));

    drawWifiIcon(218, 20);
    display.setCursor(238, 27);
    display.print(F("93%"));
    drawBatteryIcon(270, 19);

    drawBookIcon(150, 146);

    display.setFont(&FreeSerif18pt7b);
    printCentered("Reader", 230);

    display.setFont(&FreeSerif9pt7b);
    printCentered("Your library, anywhere.", 255);

    display.setFont(nullptr);
    display.setTextSize(1);
    printCentered("Press any button to continue", 356);
}

void setup()
{
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
    SPI.begin(
        Hardware::EPD_SCK,
        -1,
        Hardware::EPD_MOSI,
        Hardware::EPD_CS
    );
#endif

    display.init(115200, true, 2, false);
    display.setRotation(3); // 300 x 400 portrait
    display.setFullWindow();
    display.firstPage();
    do
    {
        drawSplash();
    }
    while (display.nextPage());

    display.hibernate();
}

void loop()
{
    // Static proof-of-concept screen.
}

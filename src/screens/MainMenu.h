#pragma once

#include <Arduino.h>

enum class MainMenuItem : uint8_t
{
    ContinueReading,
    MyBooks,
    AddBooks,
    WiFiSettings,
};

void drawMainMenu(uint8_t batteryPercent);

bool moveMainMenuUp();
bool moveMainMenuDown();
void redrawMainMenuSelection(MainMenuItem previousItem);

MainMenuItem getSelectedMainMenuItem();

#pragma once

#include <Arduino.h>

struct InputState
{
    bool upPressed;
    bool downPressed;
    bool selectPressed;
};

void initInput();
InputState readInput();

#pragma once

#include <Arduino.h>

#include "Input.h"

enum class PageId : uint8_t
{
#define PAGE(id, title, instance) id,
#include "navigation/PageRegistry.def"
#undef PAGE
    Count
};

enum class NavigationMode : uint8_t
{
    None,
    Push,
    Replace,
    Home
};

struct NavigationRequest
{
    NavigationMode mode;
    PageId destination;
};

constexpr uint8_t MAX_NAVIGATION_OPTIONS = 8;

constexpr NavigationRequest noNavigation()
{
    return { NavigationMode::None, PageId::MainMenu };
}

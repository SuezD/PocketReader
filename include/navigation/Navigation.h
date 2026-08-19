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
    Home,
    Pop,
    PopTo
};

struct NavigationRequest
{
    NavigationMode mode;
    PageId destination;
};

constexpr uint8_t MAX_NAVIGATION_OPTIONS = 8;

constexpr NavigationRequest noNavigation()
{
    return { NavigationMode::None, PageId::Count };
}

constexpr NavigationRequest navigateTo(PageId destination)
{
    return { NavigationMode::Push, destination };
}

constexpr NavigationRequest replaceWith(PageId destination)
{
    return { NavigationMode::Replace, destination };
}

constexpr NavigationRequest navigateHome(PageId destination = PageId::MainMenu)
{
    return { NavigationMode::Home, destination };
}

constexpr NavigationRequest navigateBack()
{
    return { NavigationMode::Pop, PageId::Count };
}

constexpr NavigationRequest navigateBackTo(PageId destination)
{
    return { NavigationMode::PopTo, destination };
}

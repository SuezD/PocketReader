#pragma once

#if __has_include("books/DevelopmentBookServer.h")
#include "books/DevelopmentBookServer.h"
#else
namespace DevelopmentBookServer
{
    constexpr char MANIFEST_URL[] = "";
    constexpr char ROOT_CA[] = "";
}
#endif

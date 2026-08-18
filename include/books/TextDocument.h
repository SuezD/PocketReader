#pragma once

#include <stdint.h>

class TextDocument
{
public:
    virtual ~TextDocument() = default;

    virtual bool isOpen() const = 0;
    virtual uint32_t length() const = 0;
    virtual char readCharacter(uint32_t position) const = 0;
};

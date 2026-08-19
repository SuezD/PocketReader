#include "StorageText.h"

#include <stdint.h>
#include <stdio.h>

void formatStorageSize(size_t bytes, char* output, size_t outputSize)
{
    constexpr size_t KILOBYTE = 1024;
    constexpr size_t MEGABYTE = KILOBYTE * 1024;

    if (bytes < MEGABYTE)
    {
        snprintf(
            output,
            outputSize,
            "%u KB",
            static_cast<unsigned>((bytes + KILOBYTE - 1) / KILOBYTE)
        );
        return;
    }

    const uint64_t tenths =
        (static_cast<uint64_t>(bytes) * 10 + MEGABYTE / 2) /
        MEGABYTE;
    snprintf(
        output,
        outputSize,
        "%u.%u MB",
        static_cast<unsigned>(tenths / 10),
        static_cast<unsigned>(tenths % 10)
    );
}

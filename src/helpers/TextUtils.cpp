#include "TextUtils.h"

#include <string.h>

#include "Display.h"

uint16_t getTextWidth(
    const char* text,
    const GFXfont* font
) {
    if (text == nullptr || text[0] == '\0') {
        return 0;
    }

    display.setFont(font);

    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;

    display.getTextBounds(
        text,
        0,
        0,
        &x,
        &y,
        &width,
        &height
    );

    return width;
}

void truncateToWidth(
    const char* source,
    char* output,
    size_t outputSize,
    uint16_t maximumWidth,
    const GFXfont* font
) {
    output[0] = '\0';

    if (
        source == nullptr ||
        source[0] == '\0' ||
        outputSize == 0
    ) {
        return;
    }

    const size_t sourceLength = strlen(source);
    size_t copiedLength = sourceLength;

    if (copiedLength >= outputSize) {
        copiedLength = outputSize - 1;
    }

    memcpy(output, source, copiedLength);
    output[copiedLength] = '\0';

    if (
        copiedLength == sourceLength &&
        getTextWidth(output, font) <= maximumWidth
    ) {
        return;
    }

    const char* ellipsis = "...";
    const size_t ellipsisLength = strlen(ellipsis);
    const uint16_t ellipsisWidth =
        getTextWidth(ellipsis, font);

    if (
        ellipsisWidth > maximumWidth ||
        ellipsisLength >= outputSize
    ) {
        output[0] = '\0';
        return;
    }

    while (
        copiedLength > 0 &&
        (
            copiedLength + ellipsisLength >= outputSize ||
            getTextWidth(output, font) + ellipsisWidth >
                maximumWidth
        )
    ) {
        copiedLength--;
        output[copiedLength] = '\0';
    }

    strcat(output, ellipsis);
}

#include "Selection.h"

void selectPrevious(uint8_t& selectedIndex, uint8_t optionCount)
{
    if (optionCount == 0) return;
    selectedIndex = selectedIndex == 0
        ? optionCount - 1
        : selectedIndex - 1;
}

void selectNext(uint8_t& selectedIndex, uint8_t optionCount)
{
    if (optionCount == 0) return;
    selectedIndex = selectedIndex + 1 >= optionCount
        ? 0
        : selectedIndex + 1;
}

bool moveSelection(
    const InputState& input,
    uint8_t& selectedIndex,
    uint8_t optionCount
) {
    if (optionCount == 0) return false;
    if (input.upPressed && !input.downPressed)
    {
        selectPrevious(selectedIndex, optionCount);
        return true;
    }
    if (input.downPressed && !input.upPressed)
    {
        selectNext(selectedIndex, optionCount);
        return true;
    }
    return false;
}

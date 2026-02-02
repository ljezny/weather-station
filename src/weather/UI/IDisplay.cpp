#include "IDisplay.hpp"

// Static member implementation - shared across all display implementations
FontScale_t IDisplay::currentFontScale = FONT_SCALE_NORMAL;

void IDisplay::setFontScale(FontScale_t scale)
{
    currentFontScale = scale;
    Serial.println("[IDisplay] Font scale set to: " + String((int)scale));
}

FontScale_t IDisplay::getFontScale()
{
    return currentFontScale;
}

int IDisplay::getSpacing()
{
    switch (currentFontScale) {
        case FONT_SCALE_SMALL: return 6;
        case FONT_SCALE_LARGE: return 12;
        default: return 8;
    }
}

int IDisplay::getMargin()
{
    switch (currentFontScale) {
        case FONT_SCALE_SMALL: return 3;
        case FONT_SCALE_LARGE: return 6;
        default: return 4;
    }
}

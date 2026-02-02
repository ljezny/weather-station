#pragma once
#include <Arduino.h>
#include "../Storage/WeatherConfiguration.hpp"

// =============================================================================
// Common Display Types - shared by all display implementations
// =============================================================================

enum Alignment
{
    LEADING,
    TRAILING,
    CENTER
};

enum Font {
    EXTRA_SMALL,
    TINY,
    SMALL,
    DAY_NUMBER,
    MEDIUM,
    LARGE,
    EXTRA_LARGE,
    HUGE
};

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rectangle_t;

// Unified color definitions - implementations will map these appropriately
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800       // Maps to dithered dark grey on 10.2" display
#define COLOR_GRAY 0x5AEB      // Dark gray (75% black dither on e-paper) - between VERYDARK and DARKGREY

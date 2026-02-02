#!/usr/bin/env python3
"""Convert PNG icons to C header file for e-paper display."""
from PIL import Image
import os

ICON_SIZE = 64
OUTPUT_FILE = "WeatherIconsData.h"

# Icon name to C variable name mapping
ICONS = {
    "clear-day_bw": "BITMAP_SUN",
    "clear-night_bw": "BITMAP_MOON", 
    "cloudy-1-day_bw": "BITMAP_PARTLY_CLOUDY",
    "cloudy_bw": "BITMAP_CLOUD",
    "rainy-1_bw": "BITMAP_DRIZZLE",
    "rainy-2_bw": "BITMAP_RAIN",
    "snowy-2_bw": "BITMAP_SNOW",
    "thunderstorms_bw": "BITMAP_THUNDER",
    "fog_bw": "BITMAP_FOG",
}

def convert_image_to_bytes(img_path, threshold=128):
    """Convert image to 1-bit packed bytes (MSB first)."""
    img = Image.open(img_path).convert('L')  # Grayscale
    img = img.resize((ICON_SIZE, ICON_SIZE), Image.LANCZOS)
    
    pixels = list(img.getdata())
    bytes_data = []
    
    for row in range(ICON_SIZE):
        for col_byte in range(ICON_SIZE // 8):
            byte_val = 0
            for bit in range(8):
                pixel_idx = row * ICON_SIZE + col_byte * 8 + bit
                # White pixel (>threshold) = 1 (icon), Black pixel = 0 (background)
                # Images are negated, so white = icon content
                if pixels[pixel_idx] > threshold:
                    byte_val |= (1 << (7 - bit))
            bytes_data.append(byte_val)
    
    return bytes_data

def format_c_array(name, data):
    """Format bytes as C array."""
    lines = [f"const uint8_t {name}[] PROGMEM = {{"]
    
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {hex_str},")
    
    lines.append("};")
    return "\n".join(lines)

def main():
    output = []
    output.append("#pragma once")
    output.append("")
    output.append("#include <Arduino.h>")
    output.append("")
    output.append("// Weather icons for e-paper display")
    output.append("// Icons are 64x64 pixels, 1-bit (black pixels = 1)")
    output.append("// Generated from Makin-Things/weather-icons SVG set")
    output.append("")
    output.append(f"#define WEATHER_ICON_SIZE {ICON_SIZE}")
    output.append(f"#define WEATHER_ICON_BYTES ({ICON_SIZE} * {ICON_SIZE} / 8)")
    output.append("")
    
    for png_name, c_name in ICONS.items():
        png_path = f"{png_name}.png"
        if os.path.exists(png_path):
            print(f"Converting {png_path} -> {c_name}")
            data = convert_image_to_bytes(png_path)
            output.append(f"// {png_name}.svg -> {c_name}")
            output.append(format_c_array(c_name, data))
            output.append("")
        else:
            print(f"Warning: {png_path} not found")
    
    # Add getWeatherIcon function
    output.append("// Weather code to icon mapping function")
    output.append("inline const uint8_t* getWeatherIcon(int weatherCode, bool isDay = true) {")
    output.append("    switch (weatherCode) {")
    output.append("        case 0:  // Clear sky")
    output.append("            return isDay ? BITMAP_SUN : BITMAP_MOON;")
    output.append("        case 1:  // Mainly clear")
    output.append("            return isDay ? BITMAP_SUN : BITMAP_MOON;")
    output.append("        case 2:  // Partly cloudy")
    output.append("            return BITMAP_PARTLY_CLOUDY;")
    output.append("        case 3:  // Overcast")
    output.append("            return BITMAP_CLOUD;")
    output.append("        case 45: // Fog")
    output.append("        case 48: // Depositing rime fog")
    output.append("            return BITMAP_FOG;")
    output.append("        case 51: // Light drizzle")
    output.append("        case 53: // Moderate drizzle")
    output.append("        case 55: // Dense drizzle")
    output.append("        case 56: // Light freezing drizzle")
    output.append("        case 57: // Dense freezing drizzle")
    output.append("            return BITMAP_DRIZZLE;")
    output.append("        case 61: // Slight rain")
    output.append("        case 63: // Moderate rain")
    output.append("        case 65: // Heavy rain")
    output.append("        case 66: // Light freezing rain")
    output.append("        case 67: // Heavy freezing rain")
    output.append("        case 80: // Slight rain showers")
    output.append("        case 81: // Moderate rain showers")
    output.append("        case 82: // Violent rain showers")
    output.append("            return BITMAP_RAIN;")
    output.append("        case 71: // Slight snow")
    output.append("        case 73: // Moderate snow")
    output.append("        case 75: // Heavy snow")
    output.append("        case 77: // Snow grains")
    output.append("        case 85: // Slight snow showers")
    output.append("        case 86: // Heavy snow showers")
    output.append("            return BITMAP_SNOW;")
    output.append("        case 95: // Thunderstorm")
    output.append("        case 96: // Thunderstorm with slight hail")
    output.append("        case 99: // Thunderstorm with heavy hail")
    output.append("            return BITMAP_THUNDER;")
    output.append("        default:")
    output.append("            return BITMAP_CLOUD;")
    output.append("    }")
    output.append("}")
    
    # Write output
    with open(OUTPUT_FILE, 'w') as f:
        f.write("\n".join(output))
    
    print(f"Generated {OUTPUT_FILE}")

if __name__ == "__main__":
    main()

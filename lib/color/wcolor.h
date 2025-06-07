#ifndef WCOLOR_H
#define WCOLOR_H

#include <Arduino.h>
#include <algorithm>


class WColor {
private:
    // Gamma correction constants
    static const float GAMMA;
    static const float INV_GAMMA;
    
    // Static initialization for lookup tables
    static void initializeLUT();

    // Helper functions (inlined for performance)
    inline uint8_t clamp(int value) const;
    inline uint8_t floatToByte(float value) const;
    inline float byteToFloat(uint8_t value) const;

    // Fast gamma correction functions using LUT
    inline float sRGBtoLinear(uint8_t srgb) const;
    inline uint8_t linearToSRGB(float linear) const;
    
    // Legacy gamma correction functions (kept for compatibility)
    float linearToGamma(float linear) const;
    float gammaToLinear(float gamma) const;

public:
    uint8_t r, g, b, a;
    
    // Constructors
    WColor();
    WColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255);
    WColor(uint32_t hex);

    // Getters (inlined for performance)
    inline uint8_t red() const { return r; }
    inline uint8_t green() const { return g; }
    inline uint8_t blue() const { return b; }
    inline uint8_t alpha() const { return a; }

    // Setters (inlined for performance)
    inline void setRed(uint8_t red) { r = red; }
    inline void setGreen(uint8_t green) { g = green; }
    inline void setBlue(uint8_t blue) { b = blue; }
    inline void setAlpha(uint8_t alpha) { a = alpha; }
    inline void setRGB(uint8_t red, uint8_t green, uint8_t blue) { 
        r = red; g = green; b = blue; 
    }
    inline void setRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) { 
        r = red; g = green; b = blue; a = alpha; 
    }

    // Conversion methods
    uint32_t toHex() const;
    uint32_t toHexWithAlpha() const;

    // NEW: High-performance scale method
    WColor scale(float factor) const;

    // Color operations (optimized with fast paths)
    WColor lerp(const WColor& other, float t) const;
    WColor blend(const WColor& other, float opacity) const;
    WColor add(const WColor& other) const;
    WColor subtract(const WColor& other) const;
    WColor multiply(const WColor& other) const;
    WColor screen(const WColor& other) const;
    WColor overlay(const WColor& other) const;
    
    // Advanced blend modes (keeping logarithmic accuracy)
    WColor softLight(const WColor& other) const;
    WColor hardLight(const WColor& other) const;
    WColor colorDodge(const WColor& other) const;
    WColor colorBurn(const WColor& other) const;
    WColor difference(const WColor& other) const;
    WColor exclusion(const WColor& other) const;

    // Utility methods (optimized)
    WColor brighten(float factor) const;
    WColor darken(float factor) const;
    WColor saturate(float factor) const;
    float getLuminance() const;
    WColor invert() const;
    WColor grayscale() const;

    // HSV methods (optimized)
    static WColor fromHSV(float hue, float saturation, float value, uint8_t alpha = 255);
    void toHSV(float& hue, float& saturation, float& value) const;
    WColor setHSV(float hue, float saturation, float value);

    // Operators
    WColor operator+(const WColor& other) const;
    WColor operator-(const WColor& other) const;
    WColor operator*(const WColor& other) const;
    WColor operator*(float factor) const;
    bool operator==(const WColor& other) const;
    bool operator!=(const WColor& other) const;

    // Static color constants
    static const WColor BLACK;
    static const WColor WHITE;
    static const WColor RED;
    static const WColor GREEN;
    static const WColor BLUE;
    static const WColor YELLOW;
    static const WColor CYAN;
    static const WColor MAGENTA;
    static const WColor ORANGE;
    static const WColor PURPLE;
    static const WColor PINK;
    static const WColor INVALID;
};

// Template specialization for constrain function (faster than Arduino's)
template<typename T>
inline T clamp(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

#endif // WCOLOR_H
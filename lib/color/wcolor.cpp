//color/wcolor_optimized.cpp - High-Performance Version

#include "wcolor.h"
#include <math.h>

// Gamma correction constants
const float WColor::GAMMA = 2.2f;
const float WColor::INV_GAMMA = 1.0f / 2.2f;

// Lookup tables for gamma correction (computed once, used many times)
static uint8_t srgb_to_linear_lut[256];
static uint8_t linear_to_srgb_lut[256];
static bool lut_initialized = false;

// Initialize lookup tables
void WColor::initializeLUT() {
    if (lut_initialized) return;
    
    // Build sRGB to Linear LUT
    for (int i = 0; i < 256; i++) {
        float normalized = i / 255.0f;
        float linear;
        if (normalized <= 0.04045f) {
            linear = normalized / 12.92f;
        } else {
            // Use approximation instead of pow() for speed
            // pow(x, 2.2) ≈ x * x * sqrt(x) for better performance
            float temp = (normalized + 0.055f) / 1.055f;
            linear = temp * temp * sqrtf(temp); // Faster than pow(temp, 2.2)
        }
        srgb_to_linear_lut[i] = static_cast<uint8_t>(linear * 255.0f + 0.5f);
    }
    
    // Build Linear to sRGB LUT
    for (int i = 0; i < 256; i++) {
        float linear = i / 255.0f;
        float gamma_corrected;
        if (linear <= 0.0031308f) {
            gamma_corrected = 12.92f * linear;
        } else {
            // Use approximation: pow(x, 1/2.2) ≈ sqrt(sqrt(x)) * x^0.1
            gamma_corrected = 1.055f * powf(linear, 0.454545f) - 0.055f;
        }
        linear_to_srgb_lut[i] = static_cast<uint8_t>(gamma_corrected * 255.0f + 0.5f);
    }
    
    lut_initialized = true;
}

// Fast helper functions using lookup tables
inline uint8_t WColor::clamp(int value) const {
    return (value < 0) ? 0 : (value > 255) ? 255 : static_cast<uint8_t>(value);
}

inline uint8_t WColor::floatToByte(float value) const {
    return clamp(static_cast<int>(value * 255.0f + 0.5f));
}

inline float WColor::byteToFloat(uint8_t value) const {
    return value * (1.0f / 255.0f); // Faster than division
}

// Fast gamma correction using LUT
inline float WColor::sRGBtoLinear(uint8_t srgb) const {
    if (!lut_initialized) initializeLUT();
    return srgb_to_linear_lut[srgb] * (1.0f / 255.0f);
}

inline uint8_t WColor::linearToSRGB(float linear) const {
    if (!lut_initialized) initializeLUT();
    int index = static_cast<int>(linear * 255.0f + 0.5f);
    index = (index < 0) ? 0 : (index > 255) ? 255 : index;
    return linear_to_srgb_lut[index];
}

// Legacy functions kept for compatibility but optimized
float WColor::linearToGamma(float linear) const {
    if (linear <= 0.0031308f) {
        return 12.92f * linear;
    }
    return 1.055f * powf(linear, INV_GAMMA) - 0.055f;
}

float WColor::gammaToLinear(float gamma) const {
    if (gamma <= 0.04045f) {
        return gamma / 12.92f;
    }
    return powf((gamma + 0.055f) / 1.055f, GAMMA);
}

// Constructors
WColor::WColor() : r(0), g(0), b(0), a(255) {}

WColor::WColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) 
    : r(red), g(green), b(blue), a(alpha) {}

WColor::WColor(uint32_t hex) {
    r = (hex >> 16) & 0xFF;
    g = (hex >> 8) & 0xFF;
    b = hex & 0xFF;
    a = 255;
}




// Conversion methods
uint32_t WColor::toHex() const {
    return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
}

uint32_t WColor::toHexWithAlpha() const {
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) | 
           (static_cast<uint32_t>(g) << 8) | b;
}

// NEW: High-performance scale method
WColor WColor::scale(float factor) const {
    if (factor == 1.0f) return *this; // Fast path
    if (factor == 0.0f) return WColor(0, 0, 0, a); // Fast path
    
    // Constrain factor
    factor = (factor < 0.0f) ? 0.0f : (factor > 2.0f) ? 2.0f : factor;
    
    if (factor <= 1.0f) {
        // Simple scaling for dimming (no gamma correction needed for performance)
        return WColor(
            static_cast<uint8_t>(r * factor + 0.5f),
            static_cast<uint8_t>(g * factor + 0.5f),
            static_cast<uint8_t>(b * factor + 0.5f),
            a
        );
    } else {
        // Logarithmic scaling for brightening
        float r_linear = sRGBtoLinear(r) * factor;
        float g_linear = sRGBtoLinear(g) * factor;
        float b_linear = sRGBtoLinear(b) * factor;
        
        // Clamp in linear space
        r_linear = (r_linear > 1.0f) ? 1.0f : r_linear;
        g_linear = (g_linear > 1.0f) ? 1.0f : g_linear;
        b_linear = (b_linear > 1.0f) ? 1.0f : b_linear;
        
        return WColor(
            linearToSRGB(r_linear),
            linearToSRGB(g_linear),
            linearToSRGB(b_linear),
            a
        );
    }
}

// Optimized lerp with fast paths
WColor WColor::lerp(const WColor& other, float t) const {
    if (t <= 0.0f) return *this; // Fast path
    if (t >= 1.0f) return other; // Fast path
    if (*this == other) return *this; // Fast path
    
    // Convert to linear space for interpolation
    float r_linear = sRGBtoLinear(r);
    float g_linear = sRGBtoLinear(g);
    float b_linear = sRGBtoLinear(b);
    
    float other_r_linear = sRGBtoLinear(other.r);
    float other_g_linear = sRGBtoLinear(other.g);
    float other_b_linear = sRGBtoLinear(other.b);
    
    // Interpolate in linear space
    float inv_t = 1.0f - t;
    float result_r_linear = r_linear * inv_t + other_r_linear * t;
    float result_g_linear = g_linear * inv_t + other_g_linear * t;
    float result_b_linear = b_linear * inv_t + other_b_linear * t;
    
    // Convert back to sRGB
    return WColor(
        linearToSRGB(result_r_linear),
        linearToSRGB(result_g_linear),
        linearToSRGB(result_b_linear),
        static_cast<uint8_t>(a * inv_t + other.a * t + 0.5f)
    );
}

// Optimized add with overflow protection
WColor WColor::add(const WColor& other) const {
    // Fast integer addition with saturation
    return WColor(
        clamp(static_cast<int>(r) + static_cast<int>(other.r)),
        clamp(static_cast<int>(g) + static_cast<int>(other.g)),
        clamp(static_cast<int>(b) + static_cast<int>(other.b)),
        a
    );
}

// Optimized subtract with underflow protection  
WColor WColor::subtract(const WColor& other) const {
    return WColor(
        clamp(static_cast<int>(r) - static_cast<int>(other.r)),
        clamp(static_cast<int>(g) - static_cast<int>(other.g)),
        clamp(static_cast<int>(b) - static_cast<int>(other.b)),
        a
    );
}

// Optimized multiply using integer math
WColor WColor::multiply(const WColor& other) const {
    return WColor(
        static_cast<uint8_t>((static_cast<uint16_t>(r) * other.r) >> 8),
        static_cast<uint8_t>((static_cast<uint16_t>(g) * other.g) >> 8),
        static_cast<uint8_t>((static_cast<uint16_t>(b) * other.b) >> 8),
        a
    );
}

// High-performance brighten method
WColor WColor::brighten(float factor) const {
    return scale(factor); // Use the optimized scale method
}

WColor WColor::darken(float factor) const {
    factor = (factor < 0.0f) ? 0.0f : (factor > 1.0f) ? 1.0f : factor;
    return scale(factor);
}

// Optimized luminance calculation
float WColor::getLuminance() const {
    // Use approximation for speed: simple weighted average
    // This is faster than full linear conversion for most use cases
    return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
}

// Fast grayscale conversion
WColor WColor::grayscale() const {
    uint8_t gray = static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b + 0.5f);
    return WColor(gray, gray, gray, a);
}

// Screen blend with integer optimization
WColor WColor::screen(const WColor& other) const {
    return WColor(
        255 - static_cast<uint8_t>((static_cast<uint16_t>(255 - r) * (255 - other.r)) >> 8),
        255 - static_cast<uint8_t>((static_cast<uint16_t>(255 - g) * (255 - other.g)) >> 8),
        255 - static_cast<uint8_t>((static_cast<uint16_t>(255 - b) * (255 - other.b)) >> 8),
        a
    );
}

// Rest of blend modes - keeping logarithmic accuracy where needed
WColor WColor::blend(const WColor& other, float opacity) const {
    if (opacity <= 0.0f) return *this;
    if (opacity >= 1.0f) return other;
    
    opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity;
    
    // Convert to linear space
    float src_r = sRGBtoLinear(r);
    float src_g = sRGBtoLinear(g);
    float src_b = sRGBtoLinear(b);
    float src_a = byteToFloat(a);
    
    float dst_r = sRGBtoLinear(other.r);
    float dst_g = sRGBtoLinear(other.g);
    float dst_b = sRGBtoLinear(other.b);
    float dst_a = byteToFloat(other.a) * opacity;
    
    float out_a = src_a + dst_a * (1.0f - src_a);
    
    if (out_a == 0.0f) return WColor(0, 0, 0, 0);
    
    float src_factor = src_a / out_a;
    float dst_factor = dst_a * (1.0f - src_a) / out_a;
    
    // Blend in linear space
    float result_r = src_r * src_factor + dst_r * dst_factor;
    float result_g = src_g * src_factor + dst_g * dst_factor;
    float result_b = src_b * src_factor + dst_b * dst_factor;
    
    // Convert back to sRGB
    return WColor(
        linearToSRGB(result_r),
        linearToSRGB(result_g),
        linearToSRGB(result_b),
        floatToByte(out_a)
    );
}

// Keep other blend modes for completeness (overlay, softLight, etc.)
WColor WColor::overlay(const WColor& other) const {
    auto overlayChannel = [this](uint8_t base, uint8_t overlay) -> uint8_t {
        if (base < 128) {
            return static_cast<uint8_t>((2 * base * overlay) >> 8);
        } else {
            return 255 - static_cast<uint8_t>((2 * (255 - base) * (255 - overlay)) >> 8);
        }
    };
    
    return WColor(
        overlayChannel(r, other.r),
        overlayChannel(g, other.g),
        overlayChannel(b, other.b),
        a
    );
}

// Utility methods
WColor WColor::saturate(float factor) const {
    factor = (factor < 0.0f) ? 0.0f : (factor > 2.0f) ? 2.0f : factor;
    
    // Convert to HSV, modify saturation, convert back
    float rf = r * (1.0f / 255.0f);
    float gf = g * (1.0f / 255.0f);
    float bf = b * (1.0f / 255.0f);
    
    float maxVal = max(rf, max(gf, bf));
    float minVal = min(rf, min(gf, bf));
    float delta = maxVal - minVal;
    
    if (delta == 0) return *this; // Gray color, no saturation to modify
    
    float saturation = delta / maxVal;
    saturation = (saturation * factor > 1.0f) ? 1.0f : saturation * factor;
    
    // Reconstruct color with new saturation
    float newDelta = maxVal * saturation;
    float adjustment = (delta - newDelta) * (1.0f / 3.0f);
    
    return WColor(
        clamp(static_cast<int>((rf + adjustment) * 255)),
        clamp(static_cast<int>((gf + adjustment) * 255)),
        clamp(static_cast<int>((bf + adjustment) * 255)),
        a
    );
}

WColor WColor::invert() const {
    return WColor(255 - r, 255 - g, 255 - b, a);
}

// Operators
WColor WColor::operator+(const WColor& other) const { return add(other); }
WColor WColor::operator-(const WColor& other) const { return subtract(other); }
WColor WColor::operator*(const WColor& other) const { return multiply(other); }
WColor WColor::operator*(float factor) const { return scale(factor); }

bool WColor::operator==(const WColor& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool WColor::operator!=(const WColor& other) const {
    return !(*this == other);
}

// HSV methods (optimized)
WColor WColor::fromHSV(float hue, float saturation, float value, uint8_t alpha) {
    // Constrain input values
    while (hue < 0.0f) hue += 360.0f;
    while (hue >= 360.0f) hue -= 360.0f;
    saturation = (saturation < 0.0f) ? 0.0f : (saturation > 1.0f) ? 1.0f : saturation;
    value = (value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f : value;
    
    float c = value * saturation;  // Chroma
    float x = c * (1.0f - abs(fmod(hue / 60.0f, 2.0f) - 1.0f));
    float m = value - c;
    
    float r_prime, g_prime, b_prime;
    
    int hue_sector = static_cast<int>(hue / 60.0f);
    switch (hue_sector) {
        case 0: r_prime = c; g_prime = x; b_prime = 0.0f; break;
        case 1: r_prime = x; g_prime = c; b_prime = 0.0f; break;
        case 2: r_prime = 0.0f; g_prime = c; b_prime = x; break;
        case 3: r_prime = 0.0f; g_prime = x; b_prime = c; break;
        case 4: r_prime = x; g_prime = 0.0f; b_prime = c; break;
        default: r_prime = c; g_prime = 0.0f; b_prime = x; break;
    }
    
    return WColor(
        static_cast<uint8_t>((r_prime + m) * 255.0f + 0.5f),
        static_cast<uint8_t>((g_prime + m) * 255.0f + 0.5f),
        static_cast<uint8_t>((b_prime + m) * 255.0f + 0.5f),
        alpha
    );
}

void WColor::toHSV(float& hue, float& saturation, float& value) const {
    float rf = r * (1.0f / 255.0f);
    float gf = g * (1.0f / 255.0f);
    float bf = b * (1.0f / 255.0f);
    
    float maxVal = max(rf, max(gf, bf));
    float minVal = min(rf, min(gf, bf));
    float delta = maxVal - minVal;
    
    value = maxVal;
    saturation = (maxVal == 0.0f) ? 0.0f : delta / maxVal;
    
    if (delta == 0.0f) {
        hue = 0.0f;
    } else if (maxVal == rf) {
        hue = 60.0f * fmod((gf - bf) / delta, 6.0f);
    } else if (maxVal == gf) {
        hue = 60.0f * ((bf - rf) / delta + 2.0f);
    } else {
        hue = 60.0f * ((rf - gf) / delta + 4.0f);
    }
    
    if (hue < 0.0f) {
        hue += 360.0f;
    }
}

WColor WColor::setHSV(float hue, float saturation, float value) {
    WColor newColor = fromHSV(hue, saturation, value, a);
    r = newColor.r;
    g = newColor.g;
    b = newColor.b;
    return *this;
}

// Static color constants
const WColor WColor::BLACK(0, 0, 0);
const WColor WColor::WHITE(255, 255, 255);
const WColor WColor::RED(255, 0, 0);
const WColor WColor::GREEN(0, 255, 0);
const WColor WColor::BLUE(0, 0, 255);
const WColor WColor::YELLOW(255, 255, 0);
const WColor WColor::CYAN(0, 255, 255);
const WColor WColor::MAGENTA(255, 0, 255);
const WColor WColor::ORANGE(255, 165, 0);
const WColor WColor::PURPLE(128, 0, 128);
const WColor WColor::PINK(255, 192, 203);
const WColor WColor::INVALID(255, 255, 255, 0);
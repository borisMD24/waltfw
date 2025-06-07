#ifndef EFFECTS_MANAGER_H
#define EFFECTS_MANAGER_H

#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <vector>
#include <wcolor.h>
#include <ArduinoJson.h>
#include <set>
#include <algorithm>
#include "utils.h"
#include <output.h>

// Forward declarations to avoid circular dependencies
class LEDStrip;
enum EffectType;
enum TransitionType;

/**
 * @brief Manages LED strip visual effects with thread-safe operations
 * 
 * The EffectsManager class provides a comprehensive system for rendering
 * various visual effects on LED strips, including smooth transitions
 * between effects and thread-safe parameter updates.
 */
class EffectsManager {
private:
    // Core references and state
    LEDStrip* strip;                    ///< Pointer to the LED strip instance
    bool isInitialized;                 ///< Initialization state flag
    uint32_t lastEffectUpdate;          ///< Timestamp of last effect update for throttling
    // Effect rendering methods (private implementation details)
    void renderRainbow();
    void renderBreathing();
    void renderWave();
    void renderSparkle();
    void renderChase();
    void renderFire();
    void renderTwinkle();
    void renderMeteor();
    
    public:
    void setEffectSpeedNonBlocking(float speed);
    float pendingSpeedUpdate;
    bool hasPendingSpeedUpdate;
    void setEffectIntensityNonBlocking(float intensity);
    float pendingIntensityUpdate;
    bool hasPendingIntensityUpdate;
    void processPendingUpdates();
    // Constructor and initialization
    explicit EffectsManager(LEDStrip* strip);
    
    // Core effect management
    void renderEffect();
    void initializeEffectData();
    
    // Effect type management
    void setEffect(EffectType effect);
    void setEffectSmooth(EffectType effect);
    void setEffectSmooth(EffectType effect, uint32_t duration);
    void setEffectSmooth(EffectType effect, uint32_t duration, TransitionType type);
    
    // Parameter management
    void setEffectSpeed(float speed);
    void setEffectSpeedSmooth(float speed);
    void setEffectIntensity(float intensity);
    void setEffectIntensitySmooth(float intensity);
    void setEffectWColors(const WColor& color1, 
                         const WColor& color2 = WColor::BLACK, 
                         const WColor& color3 = WColor::BLACK);
    void setEffectWColorsSmooth(const WColor& color1, 
                               const WColor& color2 = WColor::BLACK, 
                               const WColor& color3 = WColor::BLACK);
    
    // Transition support
    void blendEffectParameters(float factor);
    
    // Utility methods
    static EffectType parseEffectType(const char* effectName);
    
    // Getters for current state
    EffectType getCurrentEffect() const { return currentEffect; }
    float getEffectSpeed() const { return effectSpeed; }
    float getEffectIntensity() const { return effectIntensity; }
    WColor getEffectColor1() const { return effectWColor1; }
    WColor getEffectColor2() const { return effectWColor2; }
    WColor getEffectColor3() const { return effectWColor3; }
    bool getIsInitialized() const { return isInitialized; }

    // Public state variables (for compatibility with existing code)
    // TODO: Consider making these private with proper accessors
    
    // Current effect state
    EffectType currentEffect;           ///< Currently active effect type
    float effectSpeed;                  ///< Effect animation speed (0.1 - 10.0)
    float effectIntensity;              ///< Effect brightness/intensity (0.0 - 2.0)
    
    // Effect colors
    WColor effectWColor1;               ///< Primary effect color
    WColor effectWColor2;               ///< Secondary effect color  
    WColor effectWColor3;               ///< Tertiary effect color
    
    // Animation state variables
    uint32_t effectCounter;             ///< General purpose counter for time-based effects
    uint16_t chasePosition;             ///< Current position for chase effects
    uint16_t meteorPosition;            ///< Current position for meteor effect
    float breathePhase;                 ///< Phase accumulator for breathing effect
    float wavePhase;                    ///< Phase accumulator for wave effect
    
    // Effect-specific data storage
    std::vector<float> sparklePositions;    ///< Positions of active sparkles
    std::vector<uint8_t> sparkleIntensities; ///< Intensities of active sparkles
    std::vector<uint8_t> fireHeat;          ///< Heat values for fire effect

    // Constants for effect parameters
    static constexpr float MIN_SPEED = 0.1f;
    static constexpr float MAX_SPEED = 10.0f;
    static constexpr float MIN_INTENSITY = 0.0f;
    static constexpr float MAX_INTENSITY = 2.0f;
    static constexpr uint32_t EFFECT_COUNTER_RESET = 10000;
    static constexpr uint32_t MIN_FRAME_INTERVAL_MS = 16; // ~60 FPS base rate
};

// Inline utility functions for parameter validation
inline float clampSpeed(float speed) {
    return std::max(EffectsManager::MIN_SPEED, std::min(speed, EffectsManager::MAX_SPEED));
}

inline float clampIntensity(float intensity) {
    return std::max(EffectsManager::MIN_INTENSITY, std::min(intensity, EffectsManager::MAX_INTENSITY));
}

inline float clampFactor(float factor) {
    return std::max(0.0f, std::min(1.0f, factor));
}

#endif // EFFECTS_MANAGER_H
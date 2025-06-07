#include <cmath>
#include <algorithm>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <LEDStrip.h>
#include "EffectsManager.h"
#include <Adafruit_NeoPixel.h>
#include <TranstionsManager.h>

// Constructor with proper initialization
EffectsManager::EffectsManager(LEDStrip* strip) :
    strip(strip),
    effectCounter(0),
    effectWColor1(WColor::WHITE),
    effectWColor2(WColor::BLACK),
    effectWColor3(WColor::BLACK),
    chasePosition(0),
    meteorPosition(0), // Added member variable instead of static
    breathePhase(0.0f),
    wavePhase(0.0f),
    currentEffect(EFFECT_NONE),
    effectSpeed(1.0f),
    effectIntensity(1.0f),
    lastEffectUpdate(0),
    isInitialized(false)
{
    if (strip == nullptr) {
        Serial.println("ERROR: EffectsManager initialized with null strip pointer");
        return;
    }
    
    // Initialize effect data containers
    initializeEffectData();
    isInitialized = true;
}

// Safe effect rendering with proper error handling
void EffectsManager::renderEffect()
{
    if (!isInitialized || strip == nullptr) {
        Serial.println("ERROR: EffectsManager not properly initialized");
        return;
    }

    // Throttle effect updates to prevent excessive CPU usage
    uint32_t currentTime = millis();
    uint32_t minInterval = static_cast<uint32_t>(16.0f / effectSpeed); // Base 60fps, adjusted by speed
    if (currentTime - lastEffectUpdate < minInterval) {
        return;
    }
    lastEffectUpdate = currentTime;

    // Increment effect counter for time-based effects
    effectCounter++;
    if (effectCounter > 10000) { // Prevent overflow
        effectCounter = 0;
    }

    // Render current effect with error handling
    try {
        switch (currentEffect) {
            case EFFECT_RAINBOW:
                renderRainbow();
                break;
            case EFFECT_BREATHING:
                renderBreathing();
                break;
            case EFFECT_WAVE:
                renderWave();
                break;
            case EFFECT_SPARKLE:
                renderSparkle();
                break;
            case EFFECT_CHASE:
                renderChase();
                break;
            case EFFECT_FIRE:
                renderFire();
                break;
            case EFFECT_TWINKLE:
                renderTwinkle();
                break;
            case EFFECT_METEOR:
                renderMeteor();
                break;
            case EFFECT_NONE:
            default:
                // No effect - maintain current state
                break;
        }
    } catch (...) {
        Serial.println("ERROR: Exception in effect rendering");
        currentEffect = EFFECT_NONE; // Fallback to safe state
    }
}

void EffectsManager::renderRainbow()
{
    if (strip->neopixel.numPixels() == 0) return;
    
    float hueStep = 360.0f / strip->neopixel.numPixels();
    float baseHue = fmod(effectCounter * effectSpeed * 0.1f, 360.0f);

    for (uint16_t i = 0; i < strip->neopixel.numPixels(); i++) {
        float hue = fmod(baseHue + (i * hueStep), 360.0f);
        WColor color = WColor::fromHSV(hue, 1.0f, effectIntensity);
        strip->safeSetPixelWColor(i, color);
    }
}

void EffectsManager::renderBreathing()
{
    breathePhase += effectSpeed * 0.02f;
    if (breathePhase >= 2.0f * M_PI) {
        breathePhase -= 2.0f * M_PI;
    }

    float intensity = (sinf(breathePhase) + 1.0f) * 0.5f * effectIntensity;
    intensity = std::max(0.0f, std::min(1.0f, intensity)); // Clamp intensity
    
    WColor color = effectWColor1.scale(intensity);
    strip->fill(color);
}

void EffectsManager::renderWave()
{
    if (strip->neopixel.numPixels() == 0) return;
    
    wavePhase += effectSpeed * 0.02f;
    if (wavePhase >= 2.0f * M_PI) {
        wavePhase -= 2.0f * M_PI;
    }

    for (uint16_t i = 0; i < strip->neopixel.numPixels(); i++) {
        float pixelPhase = wavePhase + (i * 2.0f * M_PI / (strip->neopixel.numPixels() * 0.5f));
        float intensity = (sinf(pixelPhase) + 1.0f) * 0.5f * effectIntensity;
        intensity = std::max(0.0f, std::min(1.0f, intensity)); // Clamp intensity
        
        WColor color = strip->blendColors(effectWColor2, effectWColor1, intensity);
        strip->safeSetPixelWColor(i, color);
    }
}

void EffectsManager::renderSparkle()
{
    // Thread-safe fade operation
    if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(10))) { // Timeout to prevent deadlock
        float fadeAmount = std::max(0.01f, std::min(0.2f, 0.05f + (effectSpeed * 0.01f)));
        strip->fadeToBlack(fadeAmount);
        xSemaphoreGive(strip->stripMutex);
    } else {
        return; // Skip this frame if mutex unavailable
    }

    // Add new sparkles with controlled randomness
    uint8_t sparkleChance = static_cast<uint8_t>(std::min(50.0f, effectSpeed * 20.0f));
    if (random(100) < sparkleChance) {
        uint16_t pos = random(strip->neopixel.numPixels());
        strip->safeSetPixelWColor(pos, effectWColor1.scale(effectIntensity));
    }
}

void EffectsManager::renderChase()
{
    if (strip->neopixel.numPixels() == 0) return;
    
    // Thread-safe fade operation
    if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(10))) {
        strip->fadeToBlack(0.1f);
        xSemaphoreGive(strip->stripMutex);
    } else {
        return; // Skip this frame if mutex unavailable
    }

    uint16_t chaseLength = std::max(1, static_cast<int>(strip->neopixel.numPixels() * 0.1f));
    uint16_t headPos = chasePosition % strip->neopixel.numPixels();

    for (uint16_t i = 0; i < chaseLength; i++) {
        uint16_t pos = (headPos + strip->neopixel.numPixels() - i) % strip->neopixel.numPixels();
        float intensity = (1.0f - (static_cast<float>(i) / chaseLength)) * effectIntensity;
        intensity = std::max(0.0f, std::min(1.0f, intensity));
        
        WColor color = effectWColor1.scale(intensity);
        strip->safeSetPixelWColor(pos, color);
    }

    // Update chase position with bounds checking
    uint16_t speedStep = std::max(1, static_cast<int>(effectSpeed));
    chasePosition = (chasePosition + speedStep) % strip->neopixel.numPixels();
}

void EffectsManager::renderFire()
{
    if (fireHeat.empty() || strip->neopixel.numPixels() == 0) return;
    
    uint16_t numPixels = strip->neopixel.numPixels();
    
    // Ensure fireHeat array is the right size
    if (fireHeat.size() != numPixels) {
        fireHeat.resize(numPixels, 0);
    }

    // Cool down every cell
    for (uint16_t i = 0; i < numPixels; i++) {
        int cooldown = random(0, std::max(2, (55 * 10) / numPixels + 2));
        fireHeat[i] = (cooldown >= fireHeat[i]) ? 0 : fireHeat[i] - cooldown;
    }

    // Heat diffusion from bottom to top
    for (int k = numPixels - 1; k >= 2; k--) {
        fireHeat[k] = (fireHeat[k - 1] + fireHeat[k - 2] + fireHeat[k - 2]) / 3;
    }

    // Random ignition with controlled intensity
    uint8_t ignitionChance = static_cast<uint8_t>(std::min(200.0f, effectSpeed * 120.0f));
    if (random(255) < ignitionChance) {
        int y = random(std::min(7, static_cast<int>(numPixels)));
        int heatIncrease = random(160, 255);
        fireHeat[y] = std::min(255, fireHeat[y] + heatIncrease);
    }

    // Render fire with proper bounds checking
    for (uint16_t j = 0; j < numPixels; j++) {
        int temp = std::max(0, std::min(255, static_cast<int>(fireHeat[j])));
        WColor color;

        if (temp < 85) {
            color = WColor(temp * 3, 0, 0);
        } else if (temp < 170) {
            color = WColor(255, (temp - 85) * 3, 0);
        } else {
            color = WColor(255, 255, (temp - 170) * 3);
        }

        color = color.scale(effectIntensity);
        strip->safeSetPixelWColor(j, color);
    }
}

void EffectsManager::renderTwinkle()
{
    // Thread-safe fade operation
    if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(10))) {
        float fadeAmount = std::max(0.01f, std::min(0.1f, 0.02f + (effectSpeed * 0.005f)));
        strip->fadeToBlack(fadeAmount);
        xSemaphoreGive(strip->stripMutex);
    } else {
        return; // Skip this frame if mutex unavailable
    }

    // Add new twinkles
    uint8_t twinkleChance = static_cast<uint8_t>(std::min(30.0f, effectSpeed * 10.0f));
    if (random(100) < twinkleChance) {
        uint16_t pos = random(strip->neopixel.numPixels());
        WColor colors[] = {effectWColor1, effectWColor2, effectWColor3};
        WColor selectedColor = colors[random(3)].scale(effectIntensity);
        strip->safeSetPixelWColor(pos, selectedColor);
    }
}

void EffectsManager::renderMeteor()
{
    if (strip->neopixel.numPixels() == 0) return;
    
    // Thread-safe fade operation
    if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(10))) {
        strip->fadeToBlack(0.1f);
        xSemaphoreGive(strip->stripMutex);
    } else {
        return; // Skip this frame if mutex unavailable
    }

    uint16_t numPixels = strip->neopixel.numPixels();
    uint16_t meteorLength = std::max(1, static_cast<int>(numPixels * 0.05f));
    uint16_t totalTravel = numPixels + meteorLength;
    uint16_t headPos = meteorPosition % totalTravel;

    for (uint16_t i = 0; i < meteorLength; i++) {
        int pos = headPos - i;
        if (pos >= 0 && pos < static_cast<int>(numPixels)) {
            float intensity = (1.0f - (static_cast<float>(i) / meteorLength)) * effectIntensity;
            intensity = std::max(0.0f, std::min(1.0f, intensity));
            
            WColor color = effectWColor1.scale(intensity);
            strip->safeSetPixelWColor(static_cast<uint16_t>(pos), color);
        }
    }

    // Update meteor position with bounds checking
    uint16_t speedStep = std::max(1, static_cast<int>(effectSpeed));
    meteorPosition = (meteorPosition + speedStep) % totalTravel;
}

// Safe effect type parsing with validation
EffectType EffectsManager::parseEffectType(const char *effectName)
{
    if (effectName == nullptr) return EFFECT_NONE;
    
    String effect = String(effectName);
    effect.toLowerCase();
    effect.trim(); // Remove whitespace

    if (effect == "none") return EFFECT_NONE;
    if (effect == "rainbow") return EFFECT_RAINBOW;
    if (effect == "breathing" || effect == "breathe") return EFFECT_BREATHING;
    if (effect == "wave") return EFFECT_WAVE;
    if (effect == "sparkle") return EFFECT_SPARKLE;
    if (effect == "chase") return EFFECT_CHASE;
    if (effect == "fire") return EFFECT_FIRE;
    if (effect == "twinkle") return EFFECT_TWINKLE;
    if (effect == "meteor") return EFFECT_METEOR;

    Serial.printf("WARNING: Unknown effect type: %s\n", effectName);
    return EFFECT_NONE;
}

// Thread-safe effect management
void EffectsManager::setEffect(EffectType effect)
{
    if (!isInitialized || strip == nullptr) return;
    
    if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(100))) {
        if (currentEffect != effect) {
            currentEffect = effect;
            initializeEffectData();
            Serial.printf("Effect changed to: %d\n", static_cast<int>(effect));
        }
        xSemaphoreGive(strip->stripMutex);
    } else {
        Serial.println("WARNING: Failed to acquire mutex for setEffect");
    }
}



void EffectsManager::setEffectSpeed(float speed)
{
    if (!isInitialized) return;
    
    float clampedSpeed = std::max(0.1f, std::min(speed, 10.0f));
    
    // Increased timeout and added retry logic
    const int maxRetries = 3;
    const TickType_t timeout = pdMS_TO_TICKS(500); // Increased timeout
    
    for (int retry = 0; retry < maxRetries; retry++) {
        if (xSemaphoreTake(strip->stripMutex, timeout)) {
            effectSpeed = clampedSpeed;
            xSemaphoreGive(strip->stripMutex);
            Serial.printf("Effect speed set to: %.2f\n", clampedSpeed);
            return;
        }
        
        // Small delay before retry
        vTaskDelay(pdMS_TO_TICKS(10));
        Serial.printf("Mutex retry %d for setEffectSpeed\n", retry + 1);
    }
    
    Serial.println("ERROR: Failed to acquire mutex for setEffectSpeed after retries");
}



void EffectsManager::setEffectIntensity(float intensity)
{
    if (!isInitialized) return;
    
    float clampedIntensity = std::max(0.0f, std::min(intensity, 2.0f));
    
    // Increased timeout and added retry logic
    const int maxRetries = 3;
    const TickType_t timeout = pdMS_TO_TICKS(500);
    
    for (int retry = 0; retry < maxRetries; retry++) {
        if (xSemaphoreTake(strip->stripMutex, timeout)) {
            effectIntensity = clampedIntensity;
            xSemaphoreGive(strip->stripMutex);
            Serial.printf("Effect intensity set to: %.2f\n", clampedIntensity);
            return;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
        Serial.printf("Mutex retry %d for setEffectIntensity\n", retry + 1);
    }
    
    Serial.println("ERROR: Failed to acquire mutex for setEffectIntensity after retries");
}

void EffectsManager::setEffectWColors(const WColor &color1, const WColor &color2, const WColor &color3)
{
    if (!isInitialized) return;
    
    if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(100))) {
        effectWColor1 = color1;
        effectWColor2 = color2;
        effectWColor3 = color3;
        xSemaphoreGive(strip->stripMutex);
    } else {
        Serial.println("WARNING: Failed to acquire mutex for setEffectWColors");
    }
}

// Improved initialization with proper error handling
void EffectsManager::initializeEffectData()
{
    if (strip == nullptr || strip->neopixel.numPixels() == 0) {
        Serial.println("ERROR: Cannot initialize effect data - invalid strip");
        return;
    }

    uint16_t numPixels = strip->neopixel.numPixels();
    
    // Clear existing data
    sparklePositions.clear();
    sparkleIntensities.clear();
    fireHeat.clear();

    // Initialize effect-specific data based on current effect
    try {
        switch (currentEffect) {
            case EFFECT_SPARKLE:
            case EFFECT_TWINKLE:
                sparklePositions.reserve(numPixels / 4);
                sparkleIntensities.reserve(numPixels / 4);
                break;

            case EFFECT_FIRE:
                fireHeat.resize(numPixels, 0);
                // Pre-seed with some heat at the bottom
                for (int i = 0; i < std::min(3, static_cast<int>(numPixels)); i++) {
                    fireHeat[i] = random(50, 100);
                }
                break;

            default:
                break;
        }
    } catch (const std::exception& e) {
        Serial.printf("ERROR: Exception during effect data initialization: %s\n", e.what());
    } catch (...) {
        Serial.println("ERROR: Unknown exception during effect data initialization");
    }

    // Reset animation state
    chasePosition = 0;
    meteorPosition = 0;
    breathePhase = 0.0f;
    wavePhase = 0.0f;
    effectCounter = 0;
}

// Smooth transition methods with improved error handling
void EffectsManager::setEffectSmooth(EffectType effect)
{
    if (!isInitialized || strip == nullptr || strip->transitionsManager == nullptr) {
        setEffect(effect); // Fallback to immediate change
        return;
    }
    strip->transitionsManager->startTransition(effect);
}

void EffectsManager::setEffectSmooth(EffectType effect, uint32_t duration)
{
    if (!isInitialized || strip == nullptr || strip->transitionsManager == nullptr) {
        setEffect(effect); // Fallback to immediate change
        return;
    }
    strip->transitionsManager->startTransition(effect, duration, strip->transitionsManager->defaultTransitionType);
}

void EffectsManager::setEffectSmooth(EffectType effect, uint32_t duration, TransitionType type)
{
    if (!isInitialized || strip == nullptr || strip->transitionsManager == nullptr) {
        setEffect(effect); // Fallback to immediate change
        return;
    }
    strip->transitionsManager->startTransition(effect, duration, type);
}


void EffectsManager::setEffectSpeedSmooth(float speed)
{
    if (!isInitialized || strip == nullptr) {
        setEffectSpeed(speed);
        return;
    }

    // Check if transitions are available
    if (strip->transitionsManager == nullptr) {
        Serial.println("WARNING: TransitionsManager not available, using direct speed change");
        setEffectSpeed(speed);
        return;
    }

    float clampedSpeed = std::max(0.1f, std::min(speed, 10.0f));
    const TickType_t timeout = pdMS_TO_TICKS(500);
    
    if (xSemaphoreTake(strip->stripMutex, timeout)) {
        // Capture current state before starting transition
        strip->captureCurrentState();

        // Set up transition parameters
        strip->transitionsManager->transition.active = true;
        strip->transitionsManager->transition.startTime = millis();
        strip->transitionsManager->transition.duration = strip->transitionsManager->defaultTransitionDuration;
        strip->transitionsManager->transition.type = strip->transitionsManager->defaultTransitionType;
        strip->transitionsManager->transition.targetEffect = currentEffect;
        strip->transitionsManager->transition.targetSpeed = clampedSpeed;
        
        // Keep other parameters unchanged
        strip->transitionsManager->transition.targetIntensity = effectIntensity;
        strip->transitionsManager->transition.targetColor1 = effectWColor1;
        strip->transitionsManager->transition.targetColor2 = effectWColor2;
        strip->transitionsManager->transition.targetColor3 = effectWColor3;
        strip->transitionsManager->transition.targetBrightness = strip->neopixel.getBrightness();

        Serial.printf("Started smooth speed transition to: %.2f\n", clampedSpeed);
        xSemaphoreGive(strip->stripMutex);
    } else {
        Serial.println("WARNING: Failed to acquire mutex for setEffectSpeedSmooth, using direct change");
        setEffectSpeed(speed);
    }
}

void EffectsManager::setEffectSpeedNonBlocking(float speed)
{
    if (!isInitialized) return;
    
    float clampedSpeed = std::max(0.1f, std::min(speed, 10.0f));
    
    // Try to acquire mutex without blocking
    if (xSemaphoreTake(strip->stripMutex, 0)) {
        effectSpeed = clampedSpeed;
        xSemaphoreGive(strip->stripMutex);
        Serial.printf("Effect speed set to: %.2f (non-blocking)\n", clampedSpeed);
    } else {
        // Queue the update for later
        pendingSpeedUpdate = clampedSpeed;
        hasPendingSpeedUpdate = true;
        Serial.printf("Queued speed update to: %.2f\n", clampedSpeed);
    }
}



void EffectsManager::setEffectIntensityNonBlocking(float intensity)
{
    if (!isInitialized) return;
    
    float clampedIntensity = std::max(0.0f, std::min(intensity, 2.0f));
    
    if (xSemaphoreTake(strip->stripMutex, 0)) {
        effectIntensity = clampedIntensity;
        xSemaphoreGive(strip->stripMutex);
        Serial.printf("Effect intensity set to: %.2f (non-blocking)\n", clampedIntensity);
    } else {
        pendingIntensityUpdate = clampedIntensity;
        hasPendingIntensityUpdate = true;
        Serial.printf("Queued intensity update to: %.2f\n", clampedIntensity);
    }
}


void EffectsManager::processPendingUpdates()
{
    if (!isInitialized || strip == nullptr) return;
    
    // Only try if we have pending updates
    if (!hasPendingSpeedUpdate && !hasPendingIntensityUpdate) return;
    
    // Try to acquire mutex without blocking
    if (xSemaphoreTake(strip->stripMutex, 0)) {
        if (hasPendingSpeedUpdate) {
            effectSpeed = pendingSpeedUpdate;
            hasPendingSpeedUpdate = false;
            Serial.printf("Applied pending speed update: %.2f\n", pendingSpeedUpdate);
        }
        
        if (hasPendingIntensityUpdate) {
            effectIntensity = pendingIntensityUpdate;
            hasPendingIntensityUpdate = false;
            Serial.printf("Applied pending intensity update: %.2f\n", pendingIntensityUpdate);
        }
        
        xSemaphoreGive(strip->stripMutex);
    }
}



void EffectsManager::setEffectIntensitySmooth(float intensity)
{
    if (!isInitialized || strip == nullptr) {
        setEffectIntensity(intensity);
        return;
    }

    if (strip->transitionsManager == nullptr) {
        Serial.println("WARNING: TransitionsManager not available, using direct intensity change");
        setEffectIntensity(intensity);
        return;
    }

    float clampedIntensity = std::max(0.0f, std::min(intensity, 2.0f));
    const TickType_t timeout = pdMS_TO_TICKS(500);
    
    if (xSemaphoreTake(strip->stripMutex, timeout)) {
        strip->captureCurrentState();

        strip->transitionsManager->transition.active = true;
        strip->transitionsManager->transition.startTime = millis();
        strip->transitionsManager->transition.duration = strip->transitionsManager->defaultTransitionDuration;
        strip->transitionsManager->transition.type = strip->transitionsManager->defaultTransitionType;
        strip->transitionsManager->transition.targetEffect = currentEffect;
        strip->transitionsManager->transition.targetIntensity = clampedIntensity;
        
        // Keep other parameters unchanged
        strip->transitionsManager->transition.targetSpeed = effectSpeed;
        strip->transitionsManager->transition.targetColor1 = effectWColor1;
        strip->transitionsManager->transition.targetColor2 = effectWColor2;
        strip->transitionsManager->transition.targetColor3 = effectWColor3;
        strip->transitionsManager->transition.targetBrightness = strip->neopixel.getBrightness();

        Serial.printf("Started smooth intensity transition to: %.2f\n", clampedIntensity);
        xSemaphoreGive(strip->stripMutex);
    } else {
        Serial.println("WARNING: Failed to acquire mutex for setEffectIntensitySmooth, using direct change");
        setEffectIntensity(intensity);
    }
}

void EffectsManager::setEffectWColorsSmooth(const WColor &color1, const WColor &color2, const WColor &color3)
{
    if (!isInitialized || strip == nullptr || strip->transitionsManager == nullptr) {
        setEffectWColors(color1, color2, color3); // Fallback to immediate change
        return;
    }

    if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(100))) {
        strip->captureCurrentState();

        strip->transitionsManager->transition.active = true;
        strip->transitionsManager->transition.startTime = millis();
        strip->transitionsManager->transition.duration = strip->transitionsManager->defaultTransitionDuration;
        strip->transitionsManager->transition.type = strip->transitionsManager->defaultTransitionType;
        strip->transitionsManager->transition.targetEffect = currentEffect;
        strip->transitionsManager->transition.targetColor1 = color1;
        strip->transitionsManager->transition.targetColor2 = color2;
        strip->transitionsManager->transition.targetColor3 = color3;

        xSemaphoreGive(strip->stripMutex);
    } else {
        Serial.println("WARNING: Failed to acquire mutex for setEffectWColorsSmooth");
        setEffectWColors(color1, color2, color3); // Fallback
    }
}

// Improved parameter blending with bounds checking
void EffectsManager::blendEffectParameters(float factor)
{
    if (!isInitialized || strip == nullptr || strip->transitionsManager == nullptr) return;
    
    factor = std::max(0.0f, std::min(1.0f, factor)); // Ensure factor is in valid range

    // Blend speed with bounds checking
    float newSpeed = strip->transitionsManager->transition.sourceSpeed * (1.0f - factor) + 
                     strip->transitionsManager->transition.targetSpeed * factor;
    effectSpeed = std::max(0.1f, std::min(10.0f, newSpeed));

    // Blend intensity with bounds checking
    float newIntensity = strip->transitionsManager->transition.sourceIntensity * (1.0f - factor) + 
                         strip->transitionsManager->transition.targetIntensity * factor;
    effectIntensity = std::max(0.0f, std::min(2.0f, newIntensity));

    // Blend brightness with bounds checking
    float newBrightness = strip->transitionsManager->transition.sourceBrightness * (1.0f - factor) + 
                          strip->transitionsManager->transition.targetBrightness * factor;
    uint8_t brightness = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, newBrightness)));
    strip->neopixel.setBrightness(brightness);

    // Blend colors
    effectWColor1 = strip->blendColors(strip->transitionsManager->transition.sourceColor1, 
                                       strip->transitionsManager->transition.targetColor1, factor);
    effectWColor2 = strip->blendColors(strip->transitionsManager->transition.sourceColor2, 
                                       strip->transitionsManager->transition.targetColor2, factor);
    effectWColor3 = strip->blendColors(strip->transitionsManager->transition.sourceColor3, 
                                       strip->transitionsManager->transition.targetColor3, factor);
}
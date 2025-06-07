#include "GradientManager.h"
#include <TranstionsManager.h>
#include <EffectsManager.h>
#include <LEDStrip.h>

GradientManager::GradientManager(LEDStrip *strip):
      gradientEnabled(false),
      gradientReverse(false),
      strip(nullptr) {
    if (strip) {
        this->strip = strip;
    }
}

GradientManager::~GradientManager() {
    // Clean destructor - strip is owned by someone else, so we don't delete it
    if (strip && strip->stripMutex) {
        if (takeMutexSafely()) {
            gradientStops.clear();
            gradientEnabled = false;
            xSemaphoreGive(strip->stripMutex);
        }
    }
}

void GradientManager::setGradientSmooth(const WColor &startColor, const WColor &endColor)
{
    if (!isValidStrip()) {
        return;
    }
    setGradientSmooth(startColor, endColor, 
                     strip->transitionsManager->defaultTransitionDuration, 
                     strip->transitionsManager->defaultTransitionType);
}

void GradientManager::setGradientSmooth(const WColor &startColor, const WColor &endColor,
                                 uint32_t duration, TransitionType type)
{
    std::vector<GradientStop> stops;
    stops.reserve(2); // Pre-allocate for efficiency
    stops.emplace_back(0.0f, startColor);
    stops.emplace_back(1.0f, endColor);
    setGradientSmooth(stops, duration, type);
}

void GradientManager::setGradientSmooth(const std::vector<GradientStop> &stops,
                                 uint32_t duration, TransitionType type)
{
    if (!isValidStrip() || stops.empty()) {
        return;
    }
    
    // Validate gradient stops
    if (!validateGradientStops(stops)) {
        return;
    }
    
    strip->transitionsManager->transition.targetEffect = strip->effectsManager->currentEffect;
    
    if (!takeMutexSafely()) {
        return;
    }
    
    try {
        strip->transitionsManager->transition.active = false;
        strip->captureCurrentState();

        strip->transitionsManager->transition.active = true;
        if (!strip->isRunning) {
            strip->startRendering();
        }
        
        strip->transitionsManager->transition.startTime = millis();
        strip->transitionsManager->transition.duration = duration;
        strip->transitionsManager->transition.type = type;
        strip->transitionsManager->transition.targetEffect = EFFECT_NONE;
        strip->transitionsManager->transition.targetGradientEnabled = true;
        strip->transitionsManager->transition.targetGradientStops = stops;
        strip->transitionsManager->transition.targetGradientReverse = gradientReverse;
    } catch (...) {
        // Ensure mutex is released even if an exception occurs
    }
    
    xSemaphoreGive(strip->stripMutex);
}

void GradientManager::setGradientEnabledSmooth(bool enabled)
{
    if (!isValidStrip()) {
        return;
    }
    setGradientEnabledSmooth(enabled, 
                           strip->transitionsManager->defaultTransitionDuration, 
                           strip->transitionsManager->defaultTransitionType);
}

void GradientManager::setGradientEnabledSmooth(bool enabled,
                                        uint32_t duration, TransitionType type)
{
    if (!isValidStrip()) {
        return;
    }
    
    if (!takeMutexSafely()) {
        return;
    }
    
    try {
        strip->captureCurrentState();

        strip->transitionsManager->transition.active = true;
        strip->transitionsManager->transition.startTime = millis();
        strip->transitionsManager->transition.duration = duration;
        strip->transitionsManager->transition.type = type;
        strip->transitionsManager->transition.targetGradientEnabled = enabled;
    } catch (...) {
        // Ensure mutex is released even if an exception occurs
    }
    
    xSemaphoreGive(strip->stripMutex);
}

void GradientManager::renderGradient()
{
    if (!strip || gradientStops.empty()) {
        return;
    }

    const uint16_t numPixels = strip->neopixel.numPixels();
    if (numPixels == 0) {
        return;
    }

    const float pixelStep = (numPixels > 1) ? 1.0f / static_cast<float>(numPixels - 1) : 0.0f;
    
    for (uint16_t i = 0; i < numPixels; i++) {
        float position = static_cast<float>(i) * pixelStep;
        
        if (gradientReverse) {
            position = 1.0f - position;
        }

        WColor color = interpolateGradient(position);
        strip->safeSetPixelWColor(i, color);
    }
}

WColor GradientManager::interpolateGradient(float position)
{
    if (gradientStops.empty()) {
        return WColor::BLACK;
    }
    
    if (gradientStops.size() == 1) {
        return gradientStops[0].color;
    }

    // Clamp position to valid range
    position = std::max(0.0f, std::min(1.0f, position));

    // Find the appropriate gradient segment
    size_t leftIndex = 0;
    size_t rightIndex = gradientStops.size() - 1;

    // Handle edge cases first
    if (position <= gradientStops[0].position) {
        return gradientStops[0].color;
    }
    if (position >= gradientStops[rightIndex].position) {
        return gradientStops[rightIndex].color;
    }

    // Find the correct segment
    for (size_t i = 0; i < gradientStops.size() - 1; i++) {
        if (position >= gradientStops[i].position && position <= gradientStops[i + 1].position) {
            leftIndex = i;
            rightIndex = i + 1;
            break;
        }
    }

    const GradientStop &left = gradientStops[leftIndex];
    const GradientStop &right = gradientStops[rightIndex];

    // Handle identical positions
    if (std::abs(left.position - right.position) < 1e-6f) {
        return left.color;
    }

    float localPosition = (position - left.position) / (right.position - left.position);
    localPosition = std::max(0.0f, std::min(1.0f, localPosition)); // Extra safety clamp
    
    return strip->blendColors(left.color, right.color, localPosition);
}

void GradientManager::setGradient(const WColor &startColor, const WColor &endColor)
{
    if (!strip) {
        return;
    }
    
    if (!takeMutexSafely()) {
        return;
    }
    
    try {
        gradientStops.clear();
        gradientStops.reserve(2);
        gradientStops.emplace_back(0.0f, startColor);
        gradientStops.emplace_back(1.0f, endColor);
        gradientEnabled = true;
    } catch (...) {
        // Handle potential memory allocation failures
        gradientStops.clear();
        gradientEnabled = false;
    }
    
    xSemaphoreGive(strip->stripMutex);
}

void GradientManager::setGradient(const std::vector<GradientStop> &stops)
{
    if (!strip || stops.empty()) {
        return;
    }
    
    if (!validateGradientStops(stops)) {
        return;
    }
    
    if (!takeMutexSafely()) {
        return;
    }
    
    try {
        gradientStops = stops;
        std::sort(gradientStops.begin(), gradientStops.end());
        gradientEnabled = true;
    } catch (...) {
        // Handle potential memory allocation or sorting failures
        gradientStops.clear();
        gradientEnabled = false;
    }
    
    xSemaphoreGive(strip->stripMutex);
}

void GradientManager::addGradientStop(float position, const WColor &color)
{
    if (!strip) {
        return;
    }
    
    // Validate position
    if (position < 0.0f || position > 1.0f) {
        return;
    }
    
    if (!takeMutexSafely()) {
        return;
    }
    
    try {
        gradientStops.emplace_back(position, color);
        std::sort(gradientStops.begin(), gradientStops.end());
    } catch (...) {
        // Handle potential memory allocation failures
        // Don't modify gradientEnabled state on failure
    }
    
    xSemaphoreGive(strip->stripMutex);
}

void GradientManager::clearGradient()
{
    if (!strip) {
        return;
    }
    
    if (!takeMutexSafely()) {
        return;
    }
    
    gradientStops.clear();
    gradientEnabled = false;
    
    xSemaphoreGive(strip->stripMutex);
}

// Private helper methods for improved reliability

bool GradientManager::isValidStrip() const
{
    return strip && 
           strip->transitionsManager && 
           strip->effectsManager && 
           strip->stripMutex;
}

bool GradientManager::takeMutexSafely() const
{
    if (!strip || !strip->stripMutex) {
        return false;
    }
    
    return xSemaphoreTake(strip->stripMutex, portMAX_DELAY) == pdTRUE;
}

bool GradientManager::validateGradientStops(const std::vector<GradientStop> &stops) const
{
    if (stops.empty()) {
        return false;
    }
    
    // Check for valid position ranges
    for (const auto &stop : stops) {
        if (stop.position < 0.0f || stop.position > 1.0f) {
            return false;
        }
    }
    
    return true;
}
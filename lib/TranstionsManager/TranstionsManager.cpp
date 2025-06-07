#include "TranstionsManager.h"
#include <LEDStrip.h>
#include <cmath>
#include <algorithm>
#include <Arduino.h>
#include <EffectsManager.h>
#include <GradientManager.h>

TranstionsManager::TranstionsManager(LEDStrip *strip):
      defaultTransitionDuration(1000),
      defaultTransitionType(TRANSITION_EASE_IN_OUT)
{
    this->strip = strip;
}
void TranstionsManager::renderTransition() {
    float progress = calculateTransitionProgress();
    bool transitionCompleted = false;

    // Check if transition just completed
    if (progress >= 1.0f) {
        progress = 1.0f; // Clamp to exactly 1.0
        transitionCompleted = true;
        
        // Update state immediately
        strip->effectsManager->currentEffect = transition.targetEffect;
        strip->effectsManager->effectWColor1 = transition.targetColor1;
        strip->effectsManager->effectWColor2 = transition.targetColor2;
        strip->effectsManager->effectWColor3 = transition.targetColor3;
        strip->effectsManager->effectSpeed = transition.targetSpeed;
        strip->effectsManager->effectIntensity = transition.targetIntensity;
        strip->neopixel.setBrightness(transition.targetBrightness);
        strip->gradientManager->gradientEnabled = transition.targetGradientEnabled;
        strip->gradientManager->gradientStops = transition.targetGradientStops;
        strip->gradientManager->gradientReverse = transition.targetGradientReverse;
    }

    // Apply easing function
    float easedProgress = applyEasing(progress, transition.type);

    // Blend effect parameters
    strip->effectsManager->blendEffectParameters(easedProgress);

    // Create blended frame
    if (transition.sourceEffect == EFFECT_NONE && transition.targetEffect == EFFECT_NONE) {
        for (uint16_t i = 0; i < strip->neopixel.numPixels(); i++) {
            WColor sourceColor = (i < transition.sourcePixels.size()) 
                ? transition.sourcePixels[i] 
                : WColor::BLACK;
            WColor targetColor = transition.targetColor1;
            WColor blendedColor = strip->blendColors(sourceColor, targetColor, easedProgress);
            strip->safeSetPixelWColor(i, blendedColor);
        }
    }
    else if (transition.targetEffect == EFFECT_NONE) {
        for (uint16_t i = 0; i < strip->neopixel.numPixels(); i++) {
            WColor sourceColor = (i < transition.sourcePixels.size()) 
                ? transition.sourcePixels[i] 
                : WColor::BLACK;
            WColor targetColor = transition.targetColor1;
            WColor blendedColor = strip->blendColors(sourceColor, targetColor, easedProgress);
            strip->safeSetPixelWColor(i, blendedColor);
        }
    }
    else {
        // Simplified effect-to-effect blend
        WColor blendedColor = strip->blendColors(transition.sourceColor1, transition.targetColor1, easedProgress);
        strip->fill(blendedColor);
    }

    // Blend gradient states
    bool blendGradient = transition.sourceGradientEnabled || transition.targetGradientEnabled;
    
    if (blendGradient) {
        std::vector<GradientStop> blendedStops = strip->blendGradientStops(
            transition.sourceGradientStops,
            transition.targetGradientStops,
            easedProgress);
        
        bool blendedReverse = easedProgress < 0.5f 
            ? transition.sourceGradientReverse 
            : transition.targetGradientReverse;

        // Render blended gradient
        for (uint16_t i = 0; i < strip->neopixel.numPixels(); i++) {
            float position = static_cast<float>(i) / static_cast<float>(strip->neopixel.numPixels() - 1);
            if (blendedReverse) position = 1.0f - position;
            WColor color = strip->interpolateGradient(blendedStops, position);
            strip->safeSetPixelWColor(i, color);
        }
    }

    strip->neopixel.show();

    // Handle transition completion AFTER rendering
    if (transitionCompleted) {
        transition.active = false; // Mark transition as complete
        
        // Execute callback if present
        if (transitionEndCallback) {
            JsonObject emptyObj = strip->_emptyObject; // Use empty object
            transitionEndCallback(emptyObj);
        }
    }
}

float TranstionsManager::getTransitionProgress()
{
    if (!transition.active)
        return 1.0f;

    uint32_t elapsed = millis() - transition.startTime;
    return std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(transition.duration));
}
void TranstionsManager::skipTransition()
{
    if (xSemaphoreTake(strip->stripMutex, portMAX_DELAY))
    {
        if (transition.active)
        {
            transition.active = false;
            strip->effectsManager->currentEffect = transition.targetEffect;
            strip->effectsManager->effectWColor1 = transition.targetColor1;
            strip->effectsManager->effectWColor2 = transition.targetColor2;
            strip->effectsManager->effectWColor3 = transition.targetColor3;
            strip->effectsManager->effectSpeed = transition.targetSpeed;
            strip->effectsManager->effectIntensity = transition.targetIntensity;
            strip->setBrightness(transition.targetBrightness);

            // ADD THIS: Save gradient settings
            strip->gradientManager->gradientEnabled = transition.targetGradientEnabled;
            strip->gradientManager->gradientStops = transition.targetGradientStops;
            strip->gradientManager->gradientReverse = transition.targetGradientReverse;
        }
        xSemaphoreGive(strip->stripMutex);
    }
}

void TranstionsManager::stopTransition()
{
    if (xSemaphoreTake(strip->stripMutex, portMAX_DELAY))
    {
        transition.active = false;
        xSemaphoreGive(strip->stripMutex);
    }
}

float TranstionsManager::calculateTransitionProgress()
{
    if (!transition.active)
        return 1.0f;

    uint32_t elapsed = millis() - transition.startTime;
    return std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(transition.duration));
}
float TranstionsManager::applyEasing(float t, TransitionType type)
{
    // Clamp input
    t = std::max(0.0f, std::min(1.0f, t));

    switch (type)
    {
    case TRANSITION_LINEAR:
        return t;

    case TRANSITION_EASE_IN:
        return t * t;

    case TRANSITION_EASE_OUT:
        return 1.0f - (1.0f - t) * (1.0f - t);

    case TRANSITION_EASE_IN_OUT:
        return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);

    case TRANSITION_EASE_IN_QUAD:
        return t * t;

    case TRANSITION_EASE_OUT_QUAD:
        return 1.0f - (1.0f - t) * (1.0f - t);

    case TRANSITION_EASE_IN_OUT_QUAD:
        return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);

    case TRANSITION_EASE_IN_CUBIC:
        return t * t * t;

    case TRANSITION_EASE_OUT_CUBIC:
        return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

    case TRANSITION_EASE_IN_OUT_CUBIC:
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - 4.0f * (1.0f - t) * (1.0f - t) * (1.0f - t);

    case TRANSITION_BOUNCE_OUT:
    {
        if (t < 1.0f / 2.75f)
        {
            return 7.5625f * t * t;
        }
        else if (t < 2.0f / 2.75f)
        {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        }
        else if (t < 2.5f / 2.75f)
        {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        }
        else
        {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }

    case TRANSITION_ELASTIC_OUT:
    {
        if (t == 0.0f || t == 1.0f)
            return t;
        return powf(2.0f, -10.0f * t) * sinf((t - 0.075f) * (2.0f * M_PI) / 0.3f) + 1.0f;
    }

    default:
        return t;
    }
}

void TranstionsManager::startTransition(EffectType newEffect)
{
    startTransition(newEffect, defaultTransitionDuration, defaultTransitionType);
}

void TranstionsManager::startTransition(EffectType newEffect, uint32_t duration, TransitionType type)
{
    if (xSemaphoreTake(strip->stripMutex, portMAX_DELAY))
    {
        strip->captureCurrentState();

        transition.active = true;
        if (!strip->isRunning) {
            strip->startRendering();
        }
        transition.startTime = millis();
        transition.duration = duration;
        transition.type = type;
        transition.targetEffect = newEffect;

        // Set target parameters (you might want to make these configurable)
        transition.targetColor1 = strip->effectsManager->effectWColor1;
        transition.targetColor2 = strip->effectsManager->effectWColor2;
        transition.targetColor3 = strip->effectsManager->effectWColor3;
        transition.targetSpeed = strip->effectsManager->effectSpeed;
        transition.targetIntensity = strip->effectsManager->effectIntensity;
        transition.targetBrightness = strip->neopixel.getBrightness();
        transition.targetGradientEnabled = strip->gradientManager->gradientEnabled;
        transition.targetGradientStops = strip->gradientManager->gradientStops;
        transition.targetGradientReverse = strip->gradientManager->gradientReverse;
        xSemaphoreGive(strip->stripMutex);
    }
}

void TranstionsManager::setTransitionDuration(uint32_t duration)
{
    defaultTransitionDuration = duration;
}

void TranstionsManager::setOnTransitionEnd(std::function<void(JsonObject)> callback) {
    if (xSemaphoreTake(strip->stripMutex, portMAX_DELAY)) {
        transitionEndCallback = callback;
        xSemaphoreGive(strip->stripMutex);
    }
}

TransitionType TranstionsManager::parseTransitionType(const char *transitionName)
{
    String transition = String(transitionName);
    transition.toLowerCase();

    if (transition == "linear")
        return TRANSITION_LINEAR;
    if (transition == "ease_in")
        return TRANSITION_EASE_IN;
    if (transition == "ease_out")
        return TRANSITION_EASE_OUT;
    if (transition == "ease_in_out")
        return TRANSITION_EASE_IN_OUT;
    if (transition == "ease_in_quad")
        return TRANSITION_EASE_IN_QUAD;
    if (transition == "ease_out_quad")
        return TRANSITION_EASE_OUT_QUAD;
    if (transition == "ease_in_out_quad")
        return TRANSITION_EASE_IN_OUT_QUAD;
    if (transition == "ease_in_cubic")
        return TRANSITION_EASE_IN_CUBIC;
    if (transition == "ease_out_cubic")
        return TRANSITION_EASE_OUT_CUBIC;
    if (transition == "ease_in_out_cubic")
        return TRANSITION_EASE_IN_OUT_CUBIC;
    if (transition == "bounce_out")
        return TRANSITION_BOUNCE_OUT;
    if (transition == "elastic_out")
        return TRANSITION_ELASTIC_OUT;

    return TRANSITION_EASE_IN_OUT;
}

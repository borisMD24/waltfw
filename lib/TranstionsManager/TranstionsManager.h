#ifndef TRANSTIONS_H
#define TRANSTIONS_H

#include <wcolor.h>
#include <vector>
#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <vector>
#include <wcolor.h>
#include <ArduinoJson.h>
#include <set>
#include <algorithm>  // Added for std::sort
#include "utils.h"
#include <output.h>

// Forward declarations instead of includes to avoid circular dependency
class LEDStrip;
class EffectsManager;

class TranstionsManager{
    private:
    LEDStrip *strip;
    public:
    void renderGradientTransition(float easedProgress, uint16_t numPixels);
    void renderTransitionFrame(float easedProgress, uint16_t numPixels);
    TransitionState transition;
    void renderTransition();
    float getTransitionProgress();
    
    float applyEasing(float t, TransitionType type); 
    float calculateTransitionProgress();
    void skipTransition();
    void stopTransition();
    std::function<void(JsonObject)> transitionEndCallback;  // Changed from JsonObject&
    
    void startTransition(EffectType newEffect);
    void startTransition(EffectType newEffect, uint32_t duration, TransitionType type);
    uint32_t defaultTransitionDuration;
    TransitionType defaultTransitionType;
    void setTransitionDuration(uint32_t duration);
    void setOnTransitionEnd(std::function<void(JsonObject)> callback);
    TransitionType parseTransitionType(const char* transitionName);
    TranstionsManager(LEDStrip *strip);
    
    void setTransitionType(TransitionType type) { defaultTransitionType = type; }
    uint32_t getTransitionDuration() const { return defaultTransitionDuration; }
    TransitionType getTransitionType() const { return defaultTransitionType; }
    bool isTransitioning() const { return transition.active; }

};
#endif  // TRANSTIONS_H
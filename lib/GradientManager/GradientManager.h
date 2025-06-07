#ifndef GRADIENTMANAGER_H
#define GRADIENTMANAGER_H
#include <LEDStrip.h>
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
class GradientManager
{
private:
    LEDStrip *strip;
    bool isValidStrip() const;
    bool takeMutexSafely() const;
    bool validateGradientStops(const std::vector<GradientStop> &stops) const;
public:
    GradientManager(LEDStrip *strip);
    ~GradientManager();
    
    void setGradientSmooth(const std::vector<GradientStop> &stops, 
                                uint32_t duration, TransitionType type);
    void setGradientSmooth(const WColor &startColor, const WColor &endColor, 
                                uint32_t duration, TransitionType type);
    void setGradientSmooth(const WColor &startColor, const WColor &endColor);
    
    void setGradientEnabledSmooth(bool enabled);
    void setGradientEnabledSmooth(bool enabled, 
                                      uint32_t duration, TransitionType type);
    
    bool gradientEnabled;
    bool gradientReverse;
    std::vector<GradientStop> gradientStops;
    void renderGradient();
    
    WColor interpolateGradient(float position);
    
    void setGradient(const WColor& startColor, const WColor& endColor);
    void setGradient(const std::vector<GradientStop>& stops);
    void addGradientStop(float position, const WColor& color);
    void clearGradient();
    void setGradientReverse(bool reverse) { gradientReverse = reverse; }
    void enableGradient(bool enable) { gradientEnabled = enable; }
    bool isGradientEnabled() const { return gradientEnabled; }
};

#endif
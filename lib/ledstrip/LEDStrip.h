_Pragma("once");
#ifndef LEDSTRIP_H
#define LEDSTRIP_H
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

// Forward declarations to avoid circular dependencies
class EffectsManager;
class TranstionsManager;
class GradientManager;
class LEDStripJsonParser;
enum EffectType; 

class LEDStrip: public Output{
    public:
    bool isRunning;

    void captureCurrentState();
    Adafruit_NeoPixel neopixel;
    WColor interpolateGradient(const std::vector<GradientStop>& stops, float position) {
        if (stops.empty()) return WColor::BLACK;
        if (stops.size() == 1) return stops[0].color;

        position = std::max(0.0f, std::min(1.0f, position));

        size_t leftIndex = 0;
        size_t rightIndex = stops.size() - 1;

        for (size_t i = 0; i < stops.size() - 1; i++) {
            if (position >= stops[i].position && position <= stops[i+1].position) {
                leftIndex = i;
                rightIndex = i+1;
                break;
            }
        }

        const GradientStop &left = stops[leftIndex];
        const GradientStop &right = stops[rightIndex];

        if (left.position == right.position) {
            return left.color;
        }

        float localPosition = (position - left.position) / (right.position - left.position);
        return blendColors(left.color, right.color, localPosition);
    }
    WColor blendColors(const WColor& color1, const WColor& color2, float factor);
    SemaphoreHandle_t stripMutex;
    
    // Use pointers to avoid circular dependency issues
    EffectsManager* effectsManager;
    TranstionsManager* transitionsManager;
    GradientManager* gradientManager;
    LEDStripJsonParser* ledStripJsonInterpreter;
    std::function<void()> deferredCallback;
    
    bool isLooping;
    private:
    uint16_t meteorPosition;
    void stopLoop();
    bool isCurrentlyLooping() const;
    void processCallbacks();
    bool isFinalCb = false;
    bool thenLoop = false;
    TaskHandle_t renderTaskHandle;
    uint32_t frameRate;
    uint32_t frameDelay;

    static void renderTask(void* parameter);
    void renderFrame();
    void renderEffect();
    uint32_t colorToNeoPixel(const WColor& color);
    public:
    void safeSetPixelWColor(uint16_t n, const WColor& color);
    std::vector<GradientStop> blendGradientStops(
        const std::vector<GradientStop>& stops1,
        const std::vector<GradientStop>& stops2,
        float factor)
    {
        std::set<float> positions;
        for (const auto& stop : stops1) positions.insert(stop.position);
        for (const auto& stop : stops2) positions.insert(stop.position);
        
        std::vector<GradientStop> blended;
        for (float pos : positions) {
            WColor color1 = interpolateGradient(stops1, pos);
            WColor color2 = interpolateGradient(stops2, pos);
            blended.emplace_back(pos, blendColors(color1, color2, factor));
        }
        return blended;
    }
    

    StaticJsonDocument<1> _emptyDoc;
    JsonObject _emptyObject;
    LEDStrip(uint16_t numPixels, uint8_t pin, neoPixelType type = NEO_GRB + NEO_KHZ800);
    ~LEDStrip();
    bool begin()override;
    void end()override;
    void startRendering()override;
    void stopRendering();
    bool isRenderingActive() const { return isRunning; }
    
    void setFrameRate(uint32_t fps);
    uint32_t getFrameRate() const { return frameRate; }
    
    
    // These methods will be implemented in the .cpp file to avoid circular dependency
    EffectType getCurrentEffect() const;
    float getEffectSpeed() const;
    float getEffectIntensity() const;
    
    float getTransitionProgress() const;
    
    void setPixelWColor(uint16_t n, const WColor& color);
    void setPixelWColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    void setPixelWColor(uint16_t n, uint32_t color);
    WColor getPixelWColor(uint16_t n);
    
    void fill(const WColor& color);
    void fillSmooth(const WColor& color);
    void clear();
    void clearSmooth();
    void setBrightness(uint8_t brightness);
    void setBrightnessSmooth(uint8_t brightness);
    
    void fadeToBlack(float fadeAmount = 0.1f);
    void shiftPixels(int positions);
    void mirrorHalf(bool firstHalf = true);
    
    void setTaskPriority(UBaseType_t priority);
    void setTaskCore(BaseType_t core);
    void jsonInterpreter(JsonObject& json)override;
};

#endif // LEDSTRIP_H
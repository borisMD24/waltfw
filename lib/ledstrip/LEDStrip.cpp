#include "LEDStrip.h"
#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <vector>
#include <wcolor.h>
#include <ArduinoJson.h>
#include <set>
#include "utils.h"
#include <TranstionsManager.h>
#include <EffectsManager.h>
#include <GradientManager.h>
#include <LEDStripJsonParser.h>

LEDStrip::LEDStrip(uint16_t numPixels, uint8_t pin, neoPixelType type)
    : neopixel(numPixels, pin, type),
      renderTaskHandle(nullptr),
      stripMutex(nullptr),
      isRunning(false),
      frameRate(60),
      frameDelay(1000 / 60),
      meteorPosition(0),
      effectsManager(nullptr),
      transitionsManager(nullptr),
      gradientManager(nullptr)  // Initialize this too
{
    stripMutex = xSemaphoreCreateMutex();
    
    // Create the manager objects dynamically
    effectsManager = new EffectsManager(this);
    transitionsManager = new TranstionsManager(this);
    gradientManager = new GradientManager(this);
    ledStripJsonInterpreter = new LEDStripJsonParser(this);
    
    effectsManager->initializeEffectData();
}

LEDStrip::~LEDStrip()
{
    end();
    if (stripMutex)
    {
        vSemaphoreDelete(stripMutex);
    }
}

bool LEDStrip::begin()
{
    if (!neopixel.begin())
    {
        return false;
    }

    clear();
    neopixel.show();
    return true;
}

void LEDStrip::end()
{
    stopRendering();
    if (stripMutex)
    {
        vSemaphoreDelete(stripMutex);
        stripMutex = nullptr;
    }
}

void LEDStrip::startRendering()
{
    if (isRunning)
        return;

    isRunning = true;
    xTaskCreatePinnedToCore(
        renderTask,
        "LEDRender",
        4096,
        this,
        2,
        &renderTaskHandle,
        1);
}

void LEDStrip::stopRendering() {
  if (!isRunning) return;
  isRunning = false;
  if (renderTaskHandle) {
    // Wait for task to self-terminate
    while (renderTaskHandle != nullptr) vTaskDelay(1);
  }
}
 

void LEDStrip::renderTask(void* parameter) {
    LEDStrip* strip = static_cast<LEDStrip*>(parameter);
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (strip->isRunning) {
        // Take mutex for rendering
        if (xSemaphoreTake(strip->stripMutex, pdMS_TO_TICKS(10))) {
            // Render frame
            strip->renderFrame();
            
            // Process any pending deferred callbacks AFTER rendering
            if (strip->deferredCallback) {
                auto callback = strip->deferredCallback;
                strip->deferredCallback = nullptr; // Clear before executing
                
                // Release mutex before executing callback to avoid deadlocks
                xSemaphoreGive(strip->stripMutex);
                
                // Execute callback (this may call jsonInterpreter again)
                callback();
            } else {
                xSemaphoreGive(strip->stripMutex);
            }
        }
        
        // Maintain frame rate
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(strip->frameDelay));
    }
    strip->renderTaskHandle = nullptr;  // Critical fix
    vTaskDelete(NULL);
}


void LEDStrip::renderFrame()
{
    // Handle transitions first
    if (transitionsManager->transition.active)
    {
        transitionsManager->renderTransition();
        return;
    }
    // Render gradient if enabled
    if (gradientManager->gradientEnabled)
    {
        gradientManager->renderGradient();
    }

    // Render effects
    effectsManager->renderEffect();

    // Show the result
    neopixel.show();
    effectsManager->effectCounter++;
}
void LEDStrip::processCallbacks() {
    if (deferredCallback) {
        auto callback = deferredCallback;
        deferredCallback = nullptr; // Clear before executing
        callback(); // Execute the callback
    }
}


WColor LEDStrip::blendColors(const WColor &color1, const WColor &color2, float factor)
{
    factor = std::max(0.0f, std::min(1.0f, factor));

    uint8_t r = static_cast<uint8_t>(color1.r * (1.0f - factor) + color2.r * factor);
    uint8_t g = static_cast<uint8_t>(color1.g * (1.0f - factor) + color2.g * factor);
    uint8_t b = static_cast<uint8_t>(color1.b * (1.0f - factor) + color2.b * factor);

    return WColor(r, g, b);
}

void LEDStrip::captureCurrentState()
{
    transitionsManager->transition.sourceEffect = effectsManager->currentEffect;
    transitionsManager->transition.sourceColor1 = effectsManager->effectWColor1;
    transitionsManager->transition.sourceColor2 = effectsManager->effectWColor2;
    transitionsManager->transition.sourceColor3 = effectsManager->effectWColor3;
    transitionsManager->transition.sourceSpeed = effectsManager->effectSpeed;
    transitionsManager->transition.sourceIntensity = effectsManager->effectIntensity;
    transitionsManager->transition.sourceBrightness = neopixel.getBrightness();

    // Capture current pixel states

    transitionsManager->transition.sourcePixels.clear();

    transitionsManager->transition.sourceEffect = effectsManager->currentEffect;
    transitionsManager->transition.sourcePixels.reserve(neopixel.numPixels());

    for (uint16_t i = 0; i < neopixel.numPixels(); i++)
    {
        transitionsManager->transition.sourcePixels.push_back(getPixelWColor(i));
    }
    transitionsManager->transition.sourceGradientEnabled = gradientManager->gradientEnabled;
    transitionsManager->transition.sourceGradientStops = gradientManager->gradientStops;
    transitionsManager->transition.sourceGradientReverse = gradientManager->gradientReverse;
}


void LEDStrip::fillSmooth(const WColor &color)
{
    Serial.println("fillSmooth called");
    Serial.printf("Target color: R=%d, G=%d, B=%d\n", color.r, color.g, color.b);
    Serial.printf("Transition duration: %d\n", transitionsManager->defaultTransitionDuration);

    if (xSemaphoreTake(stripMutex, portMAX_DELAY))
    {
        captureCurrentState();

        transitionsManager->transition.active = true;
        transitionsManager->transition.startTime = millis();
        transitionsManager->transition.duration = transitionsManager->defaultTransitionDuration;
        transitionsManager->transition.type = transitionsManager->defaultTransitionType;
        transitionsManager->transition.sourceEffect = effectsManager->currentEffect;
        transitionsManager->transition.targetEffect = EFFECT_NONE;
        transitionsManager->transition.targetColor1 = color;
        transitionsManager->transition.targetColor2 = color;
        transitionsManager->transition.targetColor3 = color;
        transitionsManager->transition.targetSpeed = effectsManager->effectSpeed;
        transitionsManager->transition.targetIntensity = effectsManager->effectIntensity;
        transitionsManager->transition.targetBrightness = neopixel.getBrightness();

        Serial.println("Transition started successfully");
        xSemaphoreGive(stripMutex);
    }
    else
    {
        Serial.println("Failed to acquire mutex in fillSmooth");
    }
}

void LEDStrip::clearSmooth()
{
    fillSmooth(WColor::BLACK);
}

void LEDStrip::setBrightnessSmooth(uint8_t brightness)
{
    if (xSemaphoreTake(stripMutex, portMAX_DELAY))
    {
        captureCurrentState();

        transitionsManager->transition.active = true;
        transitionsManager->transition.startTime = millis();
        transitionsManager->transition.duration = transitionsManager->defaultTransitionDuration;
        transitionsManager->transition.type = transitionsManager->defaultTransitionType;
        transitionsManager->transition.targetEffect = effectsManager->currentEffect;
        transitionsManager->transition.targetBrightness = brightness;

        xSemaphoreGive(stripMutex);
    }
}

void LEDStrip::setBrightness(uint8_t brightness)
{
    if (xSemaphoreTake(stripMutex, portMAX_DELAY))
    {
        // Disable any active transition
        transitionsManager->transition.active = false;

        // Call parent class method
        neopixel.setBrightness(brightness);

        xSemaphoreGive(stripMutex);

        // Call show() outside of mutex to avoid potential deadlock
        neopixel.show();
    }
}


// Helper methods implementation
void LEDStrip::setFrameRate(uint32_t fps)
{
    frameRate = std::max(1U, std::min(fps, 120U));
    frameDelay = 1000 / frameRate;
}


void LEDStrip::safeSetPixelWColor(uint16_t n, const WColor &color)
{
    if (n < neopixel.numPixels())
    {
        neopixel.setPixelColor(n, colorToNeoPixel(color));
    }
}

uint32_t LEDStrip::colorToNeoPixel(const WColor &color)
{
    return (static_cast<uint32_t>(color.r) << 16) |
           (static_cast<uint32_t>(color.g) << 8) |
           static_cast<uint32_t>(color.b);
}

// Direct pixel control methods
void LEDStrip::setPixelWColor(uint16_t n, const WColor &color)
{
    if (xSemaphoreTake(stripMutex, portMAX_DELAY))
    {
        safeSetPixelWColor(n, color);
        xSemaphoreGive(stripMutex);
    }
}

void LEDStrip::setPixelWColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
    setPixelWColor(n, WColor(r, g, b));
}

void LEDStrip::setPixelWColor(uint16_t n, uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    setPixelWColor(n, WColor(r, g, b));
}

WColor LEDStrip::getPixelWColor(uint16_t n)
{
    if (n >= neopixel.numPixels())
        return WColor::BLACK;

    uint32_t color =  neopixel.getPixelColor(n);
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    return WColor(r, g, b);
}

void LEDStrip::fill(const WColor &color)
{
    if (xSemaphoreTake(stripMutex, portMAX_DELAY))
    {
        for (uint16_t i = 0; i < neopixel.numPixels(); i++)
        {
            safeSetPixelWColor(i, color);
        }
        xSemaphoreGive(stripMutex);
    }
}

void LEDStrip::clear()
{
    fill(WColor::BLACK);
}

void LEDStrip::fadeToBlack(float fadeAmount)
{
        for (uint16_t i = 0; i < neopixel.numPixels(); i++)
        {
            WColor currentColor = getPixelWColor(i);  // Use unsafe version
            WColor fadedColor = WColor(
                static_cast<uint8_t>(currentColor.r * (1.0f - fadeAmount)),
                static_cast<uint8_t>(currentColor.g * (1.0f - fadeAmount)),
                static_cast<uint8_t>(currentColor.b * (1.0f - fadeAmount)));
            safeSetPixelWColor(i, fadedColor);
        }
}

void LEDStrip::shiftPixels(int positions)
{
    if (positions == 0 || neopixel.numPixels() == 0)
        return;

    if (xSemaphoreTake(stripMutex, portMAX_DELAY))
    {
        std::vector<WColor> buffer;
        buffer.reserve( neopixel.numPixels());

        // Copy current colors
        for (uint16_t i = 0; i < neopixel.numPixels(); i++)
        {
            buffer.push_back(getPixelWColor(i));
        }

        // Shift and wrap
        for (uint16_t i = 0; i < neopixel.numPixels(); i++)
        {
            int sourceIndex = (i - positions) % static_cast<int>( neopixel.numPixels());
            if (sourceIndex < 0)
                sourceIndex += neopixel.numPixels();

            safeSetPixelWColor(i, buffer[sourceIndex]);
        }

        xSemaphoreGive(stripMutex);
    }
}

void LEDStrip::mirrorHalf(bool firstHalf)
{
    if (xSemaphoreTake(stripMutex, portMAX_DELAY))
    {
        uint16_t half = neopixel.numPixels() / 2;

        if (firstHalf)
        {
            // Mirror first half to second half
            for (uint16_t i = 0; i < half; i++)
            {
                WColor color = getPixelWColor(i);
                safeSetPixelWColor( neopixel.numPixels() - 1 - i, color);
            }
        }
        else
        {
            // Mirror second half to first half
            for (uint16_t i = 0; i < half; i++)
            {
                WColor color = getPixelWColor( neopixel.numPixels() - 1 - i);
                safeSetPixelWColor(i, color);
            }
        }

        xSemaphoreGive(stripMutex);
    }
}

void LEDStrip::stopLoop() {
    if (xSemaphoreTake(stripMutex, portMAX_DELAY)) {
        isLooping = false;
        Serial.println("Loop stopped");
        xSemaphoreGive(stripMutex);
    }
}

// Method to check if currently looping
bool LEDStrip::isCurrentlyLooping() const {
    return isLooping;
}

void LEDStrip::setTaskPriority(UBaseType_t priority)
{
    if (renderTaskHandle)
    {
        vTaskPrioritySet(renderTaskHandle, priority);
    }
}

void LEDStrip::setTaskCore(BaseType_t core)
{
    // Note: Task core affinity can only be set during task creation
    // This would require stopping and restarting the rendering task
    Serial.println("Warning: setTaskCore requires task restart to take effect");
}








void LEDStrip::jsonInterpreter(JsonObject &json)
{
    Serial.println("void LEDStrip::jsonInterpreter(JsonObject &json)");
    this->ledStripJsonInterpreter->jsonInterpreter(json, true);
}

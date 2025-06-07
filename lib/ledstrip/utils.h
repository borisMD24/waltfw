#ifndef LEDSTRIP_UTILS_H
#define LEDSTRIP_UTILS_H

#include <wcolor.h>
#include <vector>

struct GradientStop {
    float position;
    WColor color;

    GradientStop() : position(0.0f), color(WColor::BLACK) {}
    GradientStop(float pos, const WColor& col) : position(pos), color(col) {}
    GradientStop(float pos, uint8_t r, uint8_t g, uint8_t b) 
        : position(pos), color(r, g, b) {}
    GradientStop(float pos, uint32_t hexColor) 
        : position(pos), color(hexColor) {}
    
    bool operator<(const GradientStop& other) const {
        return position < other.position;
    }
};

enum EffectType {
    EFFECT_NONE,
    EFFECT_RAINBOW,
    EFFECT_BREATHING,
    EFFECT_WAVE,
    EFFECT_SPARKLE,
    EFFECT_CHASE,
    EFFECT_FIRE,
    EFFECT_TWINKLE,
    EFFECT_METEOR
};

enum TransitionType {
    TRANSITION_LINEAR,
    TRANSITION_EASE_IN,
    TRANSITION_EASE_OUT,
    TRANSITION_EASE_IN_OUT,
    TRANSITION_EASE_IN_QUAD,
    TRANSITION_EASE_OUT_QUAD,
    TRANSITION_EASE_IN_OUT_QUAD,
    TRANSITION_EASE_IN_CUBIC,
    TRANSITION_EASE_OUT_CUBIC,
    TRANSITION_EASE_IN_OUT_CUBIC,
    TRANSITION_BOUNCE_OUT,
    TRANSITION_ELASTIC_OUT
};

struct TransitionState {
    bool active;
    uint32_t startTime;
    uint32_t duration;
    TransitionType type;
    
    EffectType sourceEffect;
    EffectType targetEffect;
    WColor sourceColor1, sourceColor2, sourceColor3;
    WColor targetColor1, targetColor2, targetColor3;
    float sourceSpeed, targetSpeed;
    float sourceIntensity, targetIntensity;
    uint8_t sourceBrightness, targetBrightness;
    bool sourceGradientEnabled;
    bool targetGradientEnabled;
    std::vector<GradientStop> targetGradientStops;
    bool targetGradientReverse;
    std::vector<GradientStop> sourceGradientStops;
    bool sourceGradientReverse;
    std::vector<WColor> sourcePixels;
    bool useSinglePixelMode = false;
    bool usePixelArrayMode = false;
    uint16_t targetSinglePixel = 0;
    WColor targetSinglePixelColor = WColor::BLACK;
    std::vector<WColor> targetPixels;
    
    // Error handling
    bool memoryError = false;
    uint32_t lastMemoryCheck = 0;
    
    TransitionState() : active(false), startTime(0), duration(1000), 
                       type(TRANSITION_LINEAR), sourceEffect(EFFECT_NONE), 
                       targetEffect(EFFECT_NONE), sourceSpeed(1.0f), 
                       targetSpeed(1.0f), sourceIntensity(1.0f), 
                       targetIntensity(1.0f), sourceBrightness(255), 
                       targetBrightness(255), sourceGradientEnabled(false),
                       targetGradientEnabled(false), targetGradientReverse(false),
                       sourceGradientReverse(false) {}
};

    struct QueuedCommand {
        DynamicJsonDocument* doc;
        uint32_t priority;
        uint32_t queueTime;
    };
    #define MAX_RECURSION_DEPTH 16
        struct CommandState {
        uint32_t commandId;
        bool isActive;
        uint32_t startTime;
        uint32_t timeout; // Add timeout support
        std::function<void()> completionCallback;
    };
    
    // Loop management improvements
    struct LoopState {
        bool isActive;
        uint32_t currentIteration;
        uint32_t maxIterations; // Support finite loops
        uint32_t loopStartTime;
        bool shouldBreak;
    };
    #define DEFAULT_COMMAND_TIMEOUT 2000
    #define MAX_JSON_SIZE 2048
    #define MAX_QUEUE_SIZE 20
#endif
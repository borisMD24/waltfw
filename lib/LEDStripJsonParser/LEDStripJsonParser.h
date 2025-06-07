#ifndef STRIPARSER_H
#define STRIPARSER_H
#include <LEDStrip.h>


class LEDStripJsonParser{
    public:
    LEDStripJsonParser(LEDStrip* strip);
    DynamicJsonDocument* loopJsonDoc;
    JsonObject& initialJson;
    JsonObject& nextJson;
    void clean();
    
    bool copyJsonSafely(JsonObject &source, JsonObject &destination);
    bool copyJsonArraySafely(JsonArray &source, JsonArray &destination);
    void jsonInterpreter(JsonObject& json, bool first, int depth = 0);
    void handleFillCommand(const JsonObject &fillObj);
    void handleGradientCommand(const JsonObject &gradientObj);
    void handleEffectCommand(const JsonObject &effectObj);
    void handlePixelCommands(const JsonObject &pixelsObj);
    void handleAnimationControl(const JsonObject &animObj);
    void processNextThenCommand();
    int currentThenIndex = 0;
    
    
    WColor parseColor(JsonVariant colorVar);
    WColor parseHexColor(const char* hex);
    WColor parseNamedColor(const char* name);
    DynamicJsonDocument* nextJsonDoc = nullptr;
    private:
    LEDStrip* strip;
    
    StaticJsonDocument<1> _emptyDoc;
    JsonObject _emptyObject;
};

#endif
#include "LEDStripJsonParser.h"
#include <LEDStrip.h>
#include <TranstionsManager.h>
#include <GradientManager.h>
#include <EffectsManager.h>

LEDStripJsonParser::LEDStripJsonParser(LEDStrip* strip):
      initialJson(_emptyObject),
      nextJson(_emptyObject){
    this->strip = strip;
    loopJsonDoc = new DynamicJsonDocument(0); 
}


void LEDStripJsonParser::jsonInterpreter(JsonObject &json, bool first, int depth)
{
    Serial.println("=== JSON INTERPRETER START ===");
    
    // Print the entire JSON for debugging
    String jsonStr;
    serializeJson(json, jsonStr);
    Serial.printf("Received JSON: %s\n", jsonStr.c_str());
    
    if (depth > 10) {
        Serial.println("ERROR: Maximum nesting depth exceeded");
        return;
    }
    
    Serial.printf("Processing JSON - first: %s, depth: %d\n", first ? "true" : "false", depth);
    
    // Print all keys in the JSON
    Serial.print("JSON Keys: ");
    for (JsonPair kv : json) {
        Serial.printf("'%s' ", kv.key().c_str());
    }
    Serial.println();

    // Clear any pending callback if starting new sequence
    if (first)
    {
        Serial.println("First run - clearing callbacks and state");
        
        // Clean up existing documents safely
        if (nextJsonDoc) {
            delete nextJsonDoc;
            nextJsonDoc = nullptr;
        }
        if (loopJsonDoc) {
            delete loopJsonDoc;
            loopJsonDoc = nullptr;
        }
        
        strip->transitionsManager->transitionEndCallback = nullptr;
        strip->deferredCallback = nullptr;
        strip->isLooping = false;
        currentThenIndex = 0;
        
        // Check for loop flag
        if (json.containsKey("loop") && json["loop"].as<bool>()) {
            Serial.println("Loop enabled for this sequence");
            strip->isLooping = true;
            // Store the original command for looping
            size_t docSize = measureJson(json) + 512;
            loopJsonDoc = new(std::nothrow) DynamicJsonDocument(docSize);
            if (loopJsonDoc) {
                JsonObject loopCopy = loopJsonDoc->to<JsonObject>();
                for (JsonPair kv : json) {
                    if (strcmp(kv.key().c_str(), "loop") != 0) {
                        loopCopy[kv.key()] = kv.value();
                    }
                }
                Serial.println("Loop command stored successfully");
            } else {
                Serial.println("ERROR: Failed to allocate loop document");
                strip->isLooping = false;
            }
        }
    }

    // Handle 'then' command
    if (json.containsKey("then"))
    {
        Serial.println("Processing 'then' command");
        // ... existing then handling code ...
    }
    else if (strip->isLooping && loopJsonDoc) {
        Serial.println("Setting up simple loop callback");
        // Simple loop without then commands
        strip->transitionsManager->setOnTransitionEnd([this](JsonObject obj) {
            strip->deferredCallback = [this]() {
                if (this->loopJsonDoc) {
                    JsonObject loopObj = this->loopJsonDoc->as<JsonObject>();
                    jsonInterpreter(loopObj, false);
                }
            };
        });
    }

    // Process current commands
    Serial.println("Processing individual commands:");
    
    if (json.containsKey("gradient")) {
        Serial.println("- Found gradient command");
        handleGradientCommand(json["gradient"]);
    }
    
    if (json.containsKey("effect")) {
        Serial.println("- Found effect command");
        handleEffectCommand(json["effect"]);
    }
    
    if (json.containsKey("fill")) {
        Serial.println("- Found fill command");
        handleFillCommand(json["fill"]);
    }
    
    if (json.containsKey("pixels")) {
        Serial.println("- Found pixels command");
        handlePixelCommands(json["pixels"]);
    }
    
    if (json.containsKey("animation")) {
        Serial.println("- Found animation command");
        handleAnimationControl(json["animation"]);
    }
    
    Serial.println("=== JSON INTERPRETER END ===");
}



// Fixed method to process sequential then commands from array
void LEDStripJsonParser::processNextThenCommand()
{
    if (!nextJsonDoc || !nextJsonDoc->is<JsonArray>()) {
        // Fallback to loop if no more then commands
        if (strip->isLooping && loopJsonDoc) {
            currentThenIndex = 0; // Reset index for loop
            JsonObject loopObj = loopJsonDoc->as<JsonObject>();
            jsonInterpreter(loopObj, false); // This will now properly reinitialize the then array
        }
        return;
    }
    
    JsonArray thenArray = nextJsonDoc->as<JsonArray>();
    
    if (currentThenIndex >= thenArray.size()) {
        // All commands in array processed
        if (strip->isLooping && loopJsonDoc) {
            // For looping: clean up then array and restart the whole sequence
            delete nextJsonDoc;
            nextJsonDoc = nullptr;
            currentThenIndex = 0;
            JsonObject loopObj = loopJsonDoc->as<JsonObject>();
            jsonInterpreter(loopObj, false); // This will recreate the then array and start over
        } else {
            // No loop, clean up completely
            delete nextJsonDoc;
            nextJsonDoc = nullptr;
            currentThenIndex = 0;
        }
        return;
    }
    
    // Get the current command
    JsonVariant currentCmd = thenArray[currentThenIndex];
    if (currentCmd.is<JsonObject>()) {
        JsonObject cmdObj = currentCmd.as<JsonObject>();
        currentThenIndex++;
        
        // Always set up callback for next command
        strip->transitionsManager->setOnTransitionEnd([this](JsonObject obj) {
            strip->deferredCallback = [this]() {
                this->processNextThenCommand();
            };
        });
        
        // Execute the current command
        jsonInterpreter(cmdObj, false);
    } else {
        // Skip invalid command and move to next
        currentThenIndex++;
        processNextThenCommand();
    }
}

// Safer copying function with bounds checking
bool LEDStripJsonParser::copyJsonSafely(JsonObject &source, JsonObject &destination) {
    if (!source || !destination) {
        return false;
    }
    
    size_t count = 0;
    for (JsonPair kv : source) {
        if (++count > 50) { // Prevent infinite loops
            Serial.println("WARNING: Too many JSON keys, truncating");
            break;
        }
        
        const char* key = kv.key().c_str();
        if (!key) continue;
        
        JsonVariant value = kv.value();
        
        // Handle different value types safely
        if (value.is<JsonObject>()) {
            JsonObject nestedSource = value.as<JsonObject>();
            JsonObject nestedDest = destination.createNestedObject(key);
            if (!copyJsonSafely(nestedSource, nestedDest)) {
                return false;
            }
        }
        else if (value.is<JsonArray>()) {
            JsonArray sourceArray = value.as<JsonArray>();
            JsonArray destArray = destination.createNestedArray(key);
            if (!copyJsonArraySafely(sourceArray, destArray)) {
                return false;
            }
        }
        else {
            // Copy primitive values
            destination[key] = value;
        }
    }
    return true;
}

// Safer array copying
bool LEDStripJsonParser::copyJsonArraySafely(JsonArray &source, JsonArray &destination) {
    if (!source || !destination) {
        return false;
    }
    
    size_t count = 0;
    for (JsonVariant element : source) {
        if (++count > 100) { // Prevent huge arrays
            Serial.println("WARNING: Array too large, truncating");
            break;
        }
        
        if (element.is<JsonObject>()) {
            JsonObject sourceObj = element.as<JsonObject>();
            JsonObject destObj = destination.createNestedObject();
            if (!copyJsonSafely(sourceObj, destObj)) {
                return false;
            }
        }
        else if (element.is<JsonArray>()) {
            JsonArray sourceArr = element.as<JsonArray>();
            JsonArray destArr = destination.createNestedArray();
            if (!copyJsonArraySafely(sourceArr, destArr)) {
                return false;
            }
        }
        else {
            destination.add(element);
        }
    }
    return true;
}





void LEDStripJsonParser::handleFillCommand(const JsonObject &fillObj)
{
    WColor color = parseColor(fillObj["color"]);
    if (color == WColor::INVALID)
        return;

    if (fillObj.containsKey("transitionDuration"))
    {
        uint32_t duration = fillObj["transitionDuration"].as<uint32_t>();
        strip->transitionsManager->setTransitionDuration(duration);

        TransitionType transitionType = TRANSITION_EASE_IN_OUT;
        if (fillObj.containsKey("transitionType"))
        {
            transitionType = strip->transitionsManager->parseTransitionType(fillObj["transitionType"].as<const char *>());
        }

        // Store current transition settings and restore after
        TransitionType oldType = strip->transitionsManager->defaultTransitionType;
        strip->transitionsManager->defaultTransitionType = transitionType;
        strip->fillSmooth(color);
        strip->transitionsManager->defaultTransitionType = oldType;
    }
    else
    {
        strip->fill(color);
    }
}

void LEDStripJsonParser::handleGradientCommand(const JsonObject &gradientObj)
{
    Serial.println("void LEDStripJsonParser::handleGradientCommand(const JsonObject &gradientObj)");
    // Clear gradient if requested
    if (gradientObj.containsKey("clear") && gradientObj["clear"].as<bool>())
    {
        strip->gradientManager->clearGradient();
        return;
    }

    // Set reverse flag if provided
    if (gradientObj.containsKey("reverse"))
    {
        strip->gradientManager->gradientReverse = gradientObj["reverse"].as<bool>();
    }

    // Check if smooth transition is requested
    bool smoothTransition = gradientObj.containsKey("smooth") && gradientObj["smooth"].as<bool>();

    // Get transition parameters if provided
    uint32_t duration = gradientObj.containsKey("duration") ? gradientObj["duration"].as<uint32_t>() : strip->transitionsManager->defaultTransitionDuration;

    TransitionType type = gradientObj.containsKey("easing") ? strip->transitionsManager->parseTransitionType(gradientObj["easing"].as<const char *>()) : strip->transitionsManager->defaultTransitionType;

    // Handle two-color gradient
    if (gradientObj.containsKey("start") && gradientObj.containsKey("end"))
    {
        WColor startColor = parseColor(gradientObj["start"]);
        WColor endColor = parseColor(gradientObj["end"]);

        if (startColor == WColor::INVALID || endColor == WColor::INVALID)
            return;

        if (smoothTransition)
        {
            strip->gradientManager->setGradientSmooth(startColor, endColor, duration, type);
        }
        else
        {
            strip->gradientManager->setGradient(startColor, endColor);
        }
        return;
    }

    // Handle multi-stop gradient
    if (gradientObj.containsKey("stops") && gradientObj["stops"].is<JsonArray>())
    {
        JsonArray stops = gradientObj["stops"].as<JsonArray>();
        std::vector<GradientStop> gradientStops;
        bool hasValidStops = false;

        for (JsonObject stop : stops)
        {
            if (stop.containsKey("color") && stop.containsKey("position"))
            {
                WColor color = parseColor(stop["color"]);
                float position = constrain(stop["position"].as<float>(), 0.0f, 1.0f);

                if (color != WColor::INVALID)
                {
                    gradientStops.emplace_back(position, color);
                    hasValidStops = true;
                }
            }
        }

        if (!hasValidStops)
            return;

        if (smoothTransition)
        {
            strip->gradientManager->setGradientSmooth(gradientStops, duration, type);
        }
        else
        {
            strip->gradientManager->setGradient(gradientStops);
        }
        return;
    }

    // Handle enable/disable with transition
    if (gradientObj.containsKey("enabled"))
    {
        bool enabled = gradientObj["enabled"].as<bool>();

        if (smoothTransition)
        {
            strip->gradientManager->setGradientEnabledSmooth(enabled, duration, type);
        }
        else
        {
            strip->gradientManager->gradientEnabled = enabled;
        }
    }
}

void LEDStripJsonParser::handleEffectCommand(const JsonObject &effectObj)
{
    Serial.println("handleEffectCommand called");
    
    // Effect type
    if (effectObj.containsKey("type"))
    {
        const char* effectTypeStr = effectObj["type"].as<const char*>();
        Serial.printf("Setting effect type: %s\n", effectTypeStr);
        
        EffectType effect = strip->effectsManager->parseEffectType(effectTypeStr);
        
        if (effect == EFFECT_NONE && strcmp(effectTypeStr, "none") != 0) {
            Serial.printf("WARNING: Unknown effect type: %s\n", effectTypeStr);
            return;
        }

        bool useTransition = effectObj.containsKey("transitionDuration");
        
        if (useTransition)
        {
            uint32_t duration = effectObj["transitionDuration"].as<uint32_t>();
            TransitionType transitionType = TRANSITION_EASE_IN_OUT;

            if (effectObj.containsKey("transitionType"))
            {
                transitionType = strip->transitionsManager->parseTransitionType(effectObj["transitionType"].as<const char *>());
            }

            Serial.printf("Using smooth transition: duration=%d, type=%d\n", duration, transitionType);
            strip->effectsManager->setEffectSmooth(effect, duration, transitionType);
        }
        else
        {
            Serial.println("Setting effect immediately");
            strip->effectsManager->setEffect(effect);
        }
    }

    // Effect parameters - handle these AFTER setting the effect
    if (effectObj.containsKey("speed"))
    {
        float speed = constrain(effectObj["speed"].as<float>(), 0.1f, 10.0f);
        Serial.printf("Setting effect speed: %.2f\n", speed);
        
        if (effectObj.containsKey("transitionDuration"))
        {
            strip->effectsManager->setEffectSpeedNonBlocking(speed);
        }
        else
        {
            strip->effectsManager->setEffectSpeed(speed);
        }
    }

    if (effectObj.containsKey("intensity"))
    {
        float intensity = constrain(effectObj["intensity"].as<float>(), 0.0f, 2.0f);
        Serial.printf("Setting effect intensity: %.2f\n", intensity);
        
        if (effectObj.containsKey("transitionDuration"))
        {
            strip->effectsManager->setEffectIntensitySmooth(intensity);
        }
        else
        {
            strip->effectsManager->setEffectIntensity(intensity);
        }
    }

    // Effect colors - only set if the effect actually uses them
    // Rainbow effect generates its own colors, so this might be ignored
    if (effectObj.containsKey("colors"))
    {
        JsonArray colors = effectObj["colors"].as<JsonArray>();
        WColor color1 = WColor::WHITE, color2 = WColor::BLACK, color3 = WColor::BLACK;

        if (colors.size() >= 1)
            color1 = parseColor(colors[0]);
        if (colors.size() >= 2)
            color2 = parseColor(colors[1]);
        if (colors.size() >= 3)
            color3 = parseColor(colors[2]);

        Serial.printf("Setting effect colors: RGB1=(%d,%d,%d)\n", color1.r, color1.g, color1.b);

        if (effectObj.containsKey("transitionDuration"))
        {
            strip->effectsManager->setEffectWColorsSmooth(color1, color2, color3);
        }
        else
        {
            strip->effectsManager->setEffectWColors(color1, color2, color3);
        }
    }
}

void LEDStripJsonParser::handlePixelCommands(const JsonObject &pixelsObj)
{
    if (pixelsObj.containsKey("set") && pixelsObj["set"].is<JsonArray>())
    {
        JsonArray pixelArray = pixelsObj["set"].as<JsonArray>();

        for (JsonObject pixel : pixelArray)
        {
            if (pixel.containsKey("index") && pixel.containsKey("color"))
            {
                uint16_t index = pixel["index"].as<uint16_t>();
                WColor color = parseColor(pixel["color"]);

                if (index < strip->neopixel.numPixels() && color != WColor::INVALID)
                {
                    strip->setPixelWColor(index, color);
                }
            }
        }
    }

    if (pixelsObj.containsKey("range"))
    {
        JsonObject range = pixelsObj["range"].as<JsonObject>();
        if (range.containsKey("start") && range.containsKey("end") && range.containsKey("color"))
        {
            uint16_t start = range["start"].as<uint16_t>();
            uint16_t end = range["end"].as<uint16_t>();
            WColor color = parseColor(range["color"]);

            if (color != WColor::INVALID)
            {
                for (uint16_t i = start; i <= end && i < strip->neopixel.numPixels(); i++)
                {
                    strip->setPixelWColor(i, color);
                }
            }
        }
    }
}

void LEDStripJsonParser::handleAnimationControl(const JsonObject &animObj)
{
    if (animObj.containsKey("start") && animObj["start"].as<bool>())
    {
        strip->startRendering();
    }

    if (animObj.containsKey("stop") && animObj["stop"].as<bool>())
    {
        strip->stopRendering();
    }

    if (animObj.containsKey("pause"))
    {
        // You'd need to implement pause/resume functionality
        Serial.println("Pause/resume not implemented yet");
    }
}






void LEDStripJsonParser::clean(){
    if (nextJsonDoc) {
        delete nextJsonDoc;
        nextJsonDoc = nullptr;
    }
}







WColor LEDStripJsonParser::parseColor(JsonVariant colorVar)
{
    if (colorVar.is<JsonObject>())
    {
        JsonObject colorObj = colorVar.as<JsonObject>();

        // RGB format
        if (colorObj.containsKey("r") && colorObj.containsKey("g") && colorObj.containsKey("b"))
        {
            uint8_t r = constrain(colorObj["r"].as<int>(), 0, 255);
            uint8_t g = constrain(colorObj["g"].as<int>(), 0, 255);
            uint8_t b = constrain(colorObj["b"].as<int>(), 0, 255);
            uint8_t a = colorObj.containsKey("a") ? constrain(colorObj["a"].as<int>(), 0, 255) : 255;
            return WColor(r, g, b, a);
        }

        // HSV format
        if (colorObj.containsKey("h") && colorObj.containsKey("s") && colorObj.containsKey("v"))
        {
            float h = fmod(colorObj["h"].as<float>(), 360.0f);
            if (h < 0)
                h += 360.0f;
            float s = constrain(colorObj["s"].as<float>(), 0.0f, 1.0f);
            float v = constrain(colorObj["v"].as<float>(), 0.0f, 1.0f);
            return WColor::fromHSV(h, s, v);
        }

        // Named colors
        if (colorObj.containsKey("name"))
        {
            return parseNamedColor(colorObj["name"].as<const char *>());
        }
    }

    // Hex string format
    if (colorVar.is<const char *>())
    {
        const char *hexStr = colorVar.as<const char *>();
        if (hexStr[0] == '#')
        {
            return parseHexColor(hexStr + 1);
        }
        return parseNamedColor(hexStr);
    }

    // Direct RGB integer
    if (colorVar.is<int>())
    {
        uint32_t rgb = colorVar.as<uint32_t>();
        return WColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    }

    return WColor::INVALID;
}

WColor LEDStripJsonParser::parseHexColor(const char *hex)
{
    if (strlen(hex) == 6)
    {
        uint32_t rgb = strtol(hex, nullptr, 16);
        return WColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    }
    return WColor::INVALID;
}

WColor LEDStripJsonParser::parseNamedColor(const char *name)
{
    // Convert to lowercase for comparison
    String colorName = String(name);
    colorName.toLowerCase();

    if (colorName == "red")
        return WColor::RED;
    if (colorName == "green")
        return WColor::GREEN;
    if (colorName == "blue")
        return WColor::BLUE;
    if (colorName == "white")
        return WColor::WHITE;
    if (colorName == "black")
        return WColor::BLACK;
    if (colorName == "yellow")
        return WColor::YELLOW;
    if (colorName == "cyan")
        return WColor::CYAN;
    if (colorName == "magenta")
        return WColor::MAGENTA;
    if (colorName == "orange")
        return WColor::ORANGE;
    if (colorName == "purple")
        return WColor::PURPLE;
    if (colorName == "pink")
        return WColor::PINK;

    return WColor::INVALID;
}

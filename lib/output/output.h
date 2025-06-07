#ifndef OUTPUT_H
#define OUTPUT_H
#include <Arduino.h>

// Virtual base class
class Output {
public:
    // Virtual destructor is essential for proper cleanup
    virtual ~Output() = default;
    
    // Pure virtual functions that derived classes must implement
    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual void jsonInterpreter(JsonObject& json);
    virtual void startRendering();
};

#endif
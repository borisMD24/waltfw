#ifndef DINPUT_H
#define DINPUT_H
#include <Arduino.h>

// Virtual base class
class DInput {
    public:
    std::function<void(DInput*)> onChangeCb;
    // Virtual destructor is essential for proper cleanup
    virtual ~DInput() = default;
    virtual bool getState() = 0;
    unsigned long lastcheck;
    void onChange(std::function<void(DInput*)> cb){
        this->onChangeCb = cb;
    }
    virtual void check()=0;
};

#endif
#ifndef IOWRAPPER_H
#define IOWRAPPER_H

#include <omniSourceRouter.h>
#include <vector>
#include <LEDStrip.h>
#include <digitalInput.h>

class IOWrapper{
public:
    IOWrapper(OmniSourceRouter* router);
    ~IOWrapper();
    OmniSourceRouter* router;
    std::vector<Output*> outputs;
    std::vector<DInput*> dInputs;
    void pushOutput(Output*, String uid);
    void pushDigitalInput(DInput*, String uid, std::function<void(DInput* btn)> onChangeCb);
    void check();


    // New FreeRTOS task control methods
    bool startCheckTask();
    void stopCheckTask();
    void setCheckInterval(uint32_t intervalMs);
private:
    
    TaskHandle_t checkTaskHandle;
    volatile bool isTaskRunning;
    
    // Task functions
    static void checkTaskWrapper(void *parameter);
    void checkTaskLoop();

};

#endif
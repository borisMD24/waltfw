#include "IOWrapper.h"
#include <vector>
#include <LEDStrip.h>
#include <omniSourceRouter.h>
#include <output.h>

IOWrapper::IOWrapper(OmniSourceRouter *router)
{
    this->router = router;
    checkTaskHandle = NULL;
    isTaskRunning = false;
}

IOWrapper::~IOWrapper()
{
    stopCheckTask();
}

void IOWrapper::pushOutput(Output *output, String uid)
{
    if (!output)
    {
        Serial.println("ERROR: NULL output pointer!");
        return;
    }

    Serial.println("pushOutput called, current size: " + String(outputs.size()));

    int index = outputs.size();
    outputs.push_back(output); // Use push_back instead of emplace_back

    Serial.println("Output added at index: " + String(index));

    // Store the pointer directly instead of using index
    Output *outputPtr = outputs[index];

    router->addCallback(OmniSourceRouterCallback(uid, [this, outputPtr](JsonObject &data)
                                                 {
            // Validate the pointer is still in our vector
            bool found = false;
            for (Output* out : outputs) {
                if (out == outputPtr) {
                    found = true;
                    break;
                }
            }
            if (found && outputPtr) {
                outputPtr->jsonInterpreter(data);
            } }, 60));

    Serial.println("Starting rendering...");
    if (outputs[index]->begin())
    { // Call begin() first
        outputs[index]->startRendering();
        Serial.println("Rendering started successfully");
    }
    else
    {
        Serial.println("ERROR: begin() failed!");
    }
}

void IOWrapper::pushDigitalInput(DInput *input, String uid, std::function<void(DInput* btn)> onChangeCb)
{
    int index = dInputs.size();
    dInputs.push_back(input); // Use push_back instead of emplace_back

    Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
    Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
    Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
    Serial.println("dinput added at index: " + String(index));

    // Store the pointer directly instead of using index
    DInput *inputPtr = dInputs[index];
    inputPtr->onChange(onChangeCb);

    // Start the check task if it's not already running
    if (!isTaskRunning && !dInputs.empty()) {
        startCheckTask();
    }
}

void IOWrapper::check()
{
    for (DInput *input : dInputs)
    {
        if (input)
        { // toujours vérifier qu'on n'appelle pas un null pointer
            input->check();
        }
    }
}

// Static wrapper function for FreeRTOS task
void IOWrapper::checkTaskWrapper(void *parameter)
{
    IOWrapper *instance = static_cast<IOWrapper*>(parameter);
    instance->checkTaskLoop();
}

// The actual task loop
void IOWrapper::checkTaskLoop()
{
    const TickType_t xDelay = pdMS_TO_TICKS(10); // 10ms delay = 100Hz check rate
    
    Serial.println("IOWrapper check task started");
    
    while (isTaskRunning)
    {
        // Call the check method
        check();
        
        // Delay to prevent task from consuming too much CPU
        vTaskDelay(xDelay);
    }
    
    Serial.println("IOWrapper check task ended");
    vTaskDelete(NULL); // Delete this task
}

bool IOWrapper::startCheckTask()
{
    if (isTaskRunning) {
        Serial.println("Check task already running");
        return false;
    }
    
    isTaskRunning = true;
    
    BaseType_t result = xTaskCreate(
        checkTaskWrapper,        // Task function
        "IOWrapper_Check",       // Task name
        2048,                    // Stack size (adjust as needed)
        this,                    // Parameter passed to task
        1,                       // Task priority (adjust as needed)
        &checkTaskHandle         // Task handle
    );
    
    if (result == pdPASS) {
        Serial.println("IOWrapper check task created successfully");
        return true;
    } else {
        Serial.println("Failed to create IOWrapper check task");
        isTaskRunning = false;
        return false;
    }
}

void IOWrapper::stopCheckTask()
{
    if (!isTaskRunning) {
        return;
    }
    
    isTaskRunning = false;
    
    // Wait a bit for the task to finish
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // Force delete if still exists
    if (checkTaskHandle != NULL) {
        vTaskDelete(checkTaskHandle);
        checkTaskHandle = NULL;
    }
    
    Serial.println("IOWrapper check task stopped");
}

void IOWrapper::setCheckInterval(uint32_t intervalMs)
{
    // This would require more complex implementation with a queue or notification
    // For now, you can restart the task with different timing if needed
    Serial.println("Check interval change requested: " + String(intervalMs) + "ms");
}
#ifndef PSH_BTN_H
#define PSH_BTN_H
#include <pushBtn.h>
#include<digitalInput.h>

class PushBtn: public DInput
{
private:
    int pin;
    String uid;
    bool state;
    bool lastButtonState;  // Added this missing variable
    unsigned long lastDebounceTime;
    static constexpr unsigned long debounceDelay = 50; // 50 ms, tu peux ajuster
    bool getState()override;
public:
    PushBtn(int pin);
    ~PushBtn()override;
    void check()override;
};



#endif
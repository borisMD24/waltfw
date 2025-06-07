#include "pushBtn.h"
#include <digitalInput.h>
PushBtn::PushBtn(int pin)
{
    this->pin = pin;
    this->uid = uid;
    pinMode(pin, INPUT);
}

void PushBtn::check(){
    bool current = digitalRead(pin);
    if(current != state){
        onChangeCb(this);
        state = current;
    }
}

bool PushBtn::getState(){
    return state;
}

PushBtn::~PushBtn()
{
}

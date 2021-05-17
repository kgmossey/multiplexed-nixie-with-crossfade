#include "Button.h"

void Button::init(unsigned long down, unsigned long repeat, void (*btnCallbackFunction)()) {
  pressed_counter = 0;
  pressed_trigger = down;
  repeat_counter = 0;
  repeat_trigger = repeat;
  _btnCallbackFunction = btnCallbackFunction;
}

void Button::set_current_state(ButtonStates current_state) {
  switch (current_state) {
    case up:
      pressed_counter = 0;
      repeat_counter  = 0;
      break;
    case down:
      if (pressed_counter < pressed_trigger) {
        pressed_counter++;
      } else {
        if (repeat_counter==0) {
          repeat_counter++;
          _btnCallbackFunction();
        } else {
          repeat_counter++;
          if (repeat_counter==repeat_trigger) {
            repeat_counter=0;
          }
        }
      }
      break;
  }
}

void Button::setCallback(void (*btnCallbackFunction)()) {
  _btnCallbackFunction = btnCallbackFunction;
}

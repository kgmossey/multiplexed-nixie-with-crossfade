#ifndef BUTTONLIB_H
#define BUTTONLIB_H

enum ButtonStates {
  down, up
};

class Button {

public:
  void init(unsigned long down_ticks, unsigned long repeat_ticks,
            void (*btnCallbackFunction)());
  ButtonStates get_current_state(ButtonStates current_state);
  void set_current_state(ButtonStates current_state);
  void setCallback(void (*btnCallbackFunction)());

private:
  unsigned long pressed_counter;
  unsigned long pressed_trigger;
  unsigned long repeat_counter;
  unsigned long repeat_trigger;
  void (*_btnCallbackFunction)();

};

#endif

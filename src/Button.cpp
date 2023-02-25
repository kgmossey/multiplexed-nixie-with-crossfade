#include "Button.h"
#ifdef DEBUG_MODE
  #include <Arduino.h>
#endif

Button::Button (void (*CallbackFunction)()) {
  Button_Click_Handler = CallbackFunction;
  repeat = false;
  repeat_trigger = default_repeat_trigger; // default 1 second
  repeat_array_size = 0;
}

Button::Button (void (*CallbackFunction)(), bool AllowRepeat) {
  Button_Click_Handler = CallbackFunction;
  repeat = AllowRepeat;
  repeat_trigger = default_repeat_trigger; // default 1 second
  repeat_array_size = 0;
}

Button::Button (void (*CallbackFunction)(), unsigned int *Repeat, unsigned int size) {
  Button_Click_Handler = CallbackFunction;
  repeat = true;
  repeat_trigger = Repeat; // default 1 second
  repeat_array_size = size-1;
}

void Button::handle_current_state(ButtonStates current_state) {
  button_state = current_state;

  // If the button is up, exit quickly. This is the default state, so doing it first results 
  // in the least processing
  if (current_state == up) {
    debounce_counter = 0;
    repeat_counter = 0;
    repeat_index = 0;
    wake_up_override = false;
    return;
  }

  // Then debounce the button
  if (debounce_counter < debounce_trigger) {
    debounce_counter++;
    return;
  }

  // If using the settings button to wake up from a sleep state, we don't want it to go back to sleep!
  if (wake_up_override) { return; }
  
  // Kick it out if it's already pressed and it's not supposed to repeat or 
  // long click has already triggered
  if ((!repeat && repeat_counter > 0 && !Long_Click_Handler) ||
      (Long_Click_Handler && repeat_counter > long_press_trigger)) {
    return;
  }
  #ifdef DEBUG_MODE
    //Serial.print(long_press_trigger); Serial.print (" "); Serial.print(repeat_counter); Serial.print (" "); Serial.println(repeat);
  #endif

  // Single button press, or repeat button press
  if (repeat_counter == 0) {
    repeat_counter++;
    #ifdef DEBUG_MODE
      Serial.println("Button");
    #endif
    Button_Click_Handler();
    return;
  }
  
  // Long click button down
  if (Long_Click_Handler && repeat_counter == long_press_trigger) {
    repeat_counter++;
    Long_Click_Handler();
    return;
  }
  
  // 
  if ((repeat && (repeat_counter < repeat_trigger[repeat_index])) || (Long_Click_Handler)) {
    repeat_counter++;
    return;
  } 

  if (repeat && repeat_counter == repeat_trigger[repeat_index]) {
    repeat_counter = 0; // callback function triggered when repeat = 0
  }

  //if (repeat_index < (repeat_array_size-1)) {  // instead of constantly -1, do this once at init
  if (repeat_index < repeat_array_size && repeat_counter == 0) {
    repeat_index++;
  }
}

void Button::setCallback(void (*btnCallbackFunction)()) {
  Button_Click_Handler = btnCallbackFunction;
}

void Button::setLongPressCallback(void (*CallbackFunction)(), unsigned int ticks) {
  Long_Click_Handler = CallbackFunction;
  long_press_trigger = ticks;
  repeat = false;
}

void Button::setWakeUpOverride() {
  wake_up_override = true;
}

ButtonStates Button::get_current_state() {
  return button_state;
}
#ifndef BUTTONLIB_H
#define BUTTONLIB_H

/*
  Button class usage: Instantiate Button class with one of three constructors:
    Button btn1(callback) 
    Button btn2(callback, AllowRepeat)
    Button btn3(callback, AllowRepeat, &Repeat, size)

    callback: the function that is called when the button is pressed.
    AllowRepeat: a boolean to allow the button to repeat if held down. Default is false.
    Repeat: a pointer to an array that allows for different timings before the repeat is 
            triggered.  Times are in milliseconds. An array {1000, 700, 500} will trigger 
            a second press after 1 second, a third after 0.7 seconds, and another press 
            every .5 seconds. Default Repeat value is 1 second.
    Size: the size of the Repeat array. 

    It is designed to be used with a timer that is calls
*/
enum ButtonStates {
  down, up
};

class Button {

public:
  Button(void (*CallbackFunction)());
  Button(void (*CallbackFunction)(), bool AllowRepeat);
  Button(void (*CallbackFunction)(), unsigned int *Repeat, unsigned int size);
  
  ButtonStates get_current_state(ButtonStates current_state);
  void handle_current_state(ButtonStates current_state);
  void setCallback(void (*CallbackFunction)());
  void setLongPressCallback(void (*CallbackFunction)(), unsigned int ticks);
  void setDebounce(unsigned long millis);
  void setRepeat(bool AllowRepeat, unsigned int *Repeat[], unsigned int size);
  void setWakeUpOverride();

private:
  unsigned long debounce_counter = 0;
  unsigned long debounce_trigger = 50;
  unsigned int default_repeat_trigger[1] = { 1000 };
  unsigned int repeat_counter = 0;  // number of ticks passed since either debounce or previous 
                                    // button press
  unsigned int *repeat_trigger;  // ability to pass an array 
  unsigned int repeat_index = 0; // which position in the repeat_trigger array to use
  unsigned int repeat_array_size; // size of repeat_trigger array
  unsigned int long_press_trigger;
  bool wake_up_override = false;
  bool repeat = false;

  void (*Button_Click_Handler)();
  void (*Long_Click_Handler)();

};

#endif

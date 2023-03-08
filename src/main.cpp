#include "main.h"

unsigned long start=0;
unsigned long end=0;

/********************
 * Code starts here *
 ********************/
void setup() {

  // Set button inputs
  DDRB  &= B11111110;   // Pin 14 (D8) low for input (Button #4)
  DDRD  &= B00011111;   // Pins 11-13 (D5-D7) set low for input (Buttons #1-3)
  PORTB |= B00000001;   // and write a one for internal resistor pullup
  PORTD |= B11100000;
  // Set tube outputs.  Lowercase are "ones" place driver chip.
  DDRB  |= B00111110;   // Pins 15-19 (D9-D13) set high for output (--cADBC-)
  DDRC  |= B00000111;   // Pins 23-25 (A0-A2) set high for ouput   (-----bda)
  DDRD  |= B00011100;   // Pins 2-4 (D2-D4) set high for output    (---SHM--)

  // Button delay times are .1ms
  turn_off_tubes(left + center + right);
  #ifdef DEBUG_MODE
    Serial.begin(38400);
  #endif
  clock.begin();
    
  if (!clock.isStarted()) {
    clock.fillByYMD(2022,8,19);     // Y,M,D
    clock.fillByHMS(20,13,00);      // H,M,S
    clock.fillDayOfWeek(TUE);
    clock.setTime();
    clock.setRamAddress(0, BrightnessMed); // medium brightness default time
    clock.setRamAddress(1, 0); clock.setRamAddress(2, 5); // 5 minute default off time
  } else {
    clock.getTime();
  }
  PowerState.setTrigger(GoToSleep);
  PowerState.setMaxTick ((((unsigned long)clock.getRamAddress(1)<<8) + 
                           (unsigned long)clock.getRamAddress(2)) * TICKS_PER_MINUTE);
  Brightness = clock.getRamAddress(0);
  if (Brightness<1) { Brightness = 1; }
  if (Brightness>8) { Brightness = 8; }
  // todo: get following from ram
  Settings.setLongPressCallback(GoToSleep, 3 * TICKS_PER_SECOND);
  SettingsSM.setMaxTick(10 * TICKS_PER_SECOND);
  CrossfadeSM.setMaxTick (240);
  LeftTubesOn.setNext(TubesOff_L);
  TubesOff_L.setNext(CenterTubesOn);
  CenterTubesOn.setNext(TubesOff_C);
  TubesOff_C.setNext(RightTubesOn);
  RightTubesOn.setNext(TubesOff_R);
  TubesOff_R.setNext(LeftTubesOn);
  
  SetTimerInterrupts();
  cycle_digits();
  
  // Delay enabling Timer 1 (Clock chip read) until after warmup cycle
  // to prevent reading new times and updating the display
  TIMSK1 |= _BV(OCIE1A); // Timer/Counter1 Interrupt Mask Register - Enable

}

void loop() {

  if (TC1IRQ_complete) { // 2Hz interrrupt
    // This getTime() call takes slight over 1ms, needs to be handled outside 
    // interrupt because of the way the wire library works.
    if (SettingsSM.getCurrentState() == NormalDisplay) {
      clock.getTime();
      if (DISPLAY_DATE &&
          display_date_step > DISPLAY_DATE_START &&
          display_date_step <= DISPLAY_DATE_END) {
        display.update (clock.year, clock.month, clock.dayOfMonth, Brightness);
      } else {
        display.update (clock.hour, clock.minute, clock.second, Brightness);
      }
    }
    TC1IRQ_complete = false;
  }
  
  if (TC2IRQ_complete) { // 10,000Hz interrupt
    // Handle button presses!
    Settings.handle_current_state((SettingsButton == Pressed) ? down : up);
    Advance.handle_current_state((AdvanceButton == Pressed) ? down : up);
    Decrease.handle_current_state((DecreaseButton == Pressed) ? down : up);
  
    TC2IRQ_complete = false;
  }

}

/******************************
 * Check time at 2Hz interval *
 ******************************/
ISR(TIMER1_COMPA_vect) {
  display_date_step++;
  if (display_date_step == DISPLAY_DATE_END) { display_date_step = 0; }

  if (SettingsSM.getCurrentState() == NormalDisplay) {
    display.flash = false;
  } else {
    display.flash = !display.flash;
  }
  TC1IRQ_complete = true;
}

/*****************************************************************
 * Will fire every 100uS.  See if the state needs to transition, *
 * and checks the button states as well.  - 10,000Hz -           *
 *****************************************************************/
ISR(TIMER2_COMPA_vect) {
  static byte multiplexTick;
  static byte xfadeTick;
  
  MultiplexSM.tick();
  CrossfadeSM.tick();
  PowerState.tick();
  SettingsSM.tick();
      
  multiplexTick = MultiplexSM.getTick();
  xfadeTick = CrossfadeSM.getTick();

  if (multiplexTick == 0 || multiplexTick == Brightness) {
    if (multiplexTick == 0 && display.TriggerSet) display.resetStep(Brightness);
    MultiplexSM.transitionNext();
  }

  if (display.AllowTransition && (multiplexTick > display.getXfadeStep())) {
    CrossfadeSM.transitionTo(Previous);
  } else {
    CrossfadeSM.transitionTo(Current);
  }
  MultiplexSM.update();

  // When the crossfade tick is 0, increment the crossfade pass counter
  if (xfadeTick == 0) {
    display.nextStep();
  }
 
  TC2IRQ_complete = true;
}

void turn_off_all_tubes () {
  turn_off_tubes(left + center + right);
}

void turn_off_tubes(byte tube_mask) {
  PORTD &= 255 - tube_mask;
}

void update_tube_pair (byte value, byte tube_pair){

  // Turning off the anode even though it should already be off to prevent
  // cathode switching in the case of cross-fading
  turn_off_tubes(tube_pair);
  PORTB = (bcd[value] << 1) & B00111110;
  PORTC = (bcd[value] >> 5) & B00000111;
  /* moving this code to each tube pair to account for settings state
  // Now we can turn on the tube anode, if it's not in the off cycle of a flashing event
  // and it's power state is On (ie not "Sleeping")
  if (!display.flash && PowerState.isInState(On)) {
    PORTD |= tube_pair;
  }*/

}

void update_left() {
  #ifdef DEBUG_MODE
    switch (SettingsSM.getCurrentStateId()) {
      case setminutes:
        update_center();
        return;
        break;
      case normaldisplay:
      case setseconds:
      case setyear:
      case setmonth:
      case setday:
      case setbrightness:
      case setsleep:
        update_right();
        return;
      case sethours:
        ; // do nothing
    }
  #endif
  // Check cross-fade
  update_tube_pair(CrossfadeSM.isInState(Current)
                   ? display.getCurrentLeft()
                   : display.getPreviousLeft(), left);
  if (PowerState.isInState(Sleeping)) { return; }
  switch (SettingsSM.getCurrentStateId()) {
    case normaldisplay:
    case setminutes:
    case setseconds:
    case setyear:
    case setmonth:
    case setday:
    case setbrightness:
    case setsleep:
      PORTD |= left; 
      break;
    case sethours:
      if (Advance.get_current_state() == down || Decrease.get_current_state() == down || !display.flash) { 
        PORTD |= left; 
      }
  }


}

void update_center() {
  #ifdef DEBUG_MODE
    switch (SettingsSM.getCurrentStateId()) {
      case sethours:
        update_left();
        return;  
      case normaldisplay:
      case setseconds:
      case setyear:
      case setmonth:
      case setday:
      case setbrightness:
      case setsleep:
        update_right();
        return;
      case setminutes:
        ; // do nothing
    }    
  #endif
  // Check cross-fade
  update_tube_pair(CrossfadeSM.isInState(Current)
                   ? display.getCurrentCenter()
                   : display.getPreviousCenter(), center); 
  if (PowerState.isInState(Sleeping)) { return; }
  switch (SettingsSM.getCurrentStateId()) {
    case normaldisplay:
    case sethours:
    case setminutes:
    case setseconds:
    case setyear:
      if ((SettingsSM.getCurrentState() == SetMinutes || SettingsSM.getCurrentState() == SetYear) && 
           Advance.get_current_state() == up && Decrease.get_current_state() == up && display.flash) {
          return;
        } 
      if (PowerState.isInState(On)) { PORTD |= center; }
  }
}

void update_right() {
  #ifdef DEBUG_MODE
    switch (SettingsSM.getCurrentStateId()) {
      case sethours:
        update_left();
        return;  
      case setminutes:
        update_center();
        return;
      case normaldisplay:
      case setseconds:
      case setyear:
      case setmonth:
      case setday:
      case setbrightness:
      case setsleep:
        ; // do nothing
    }    
  #endif
  // Check cross-fade
  update_tube_pair(CrossfadeSM.isInState(Current)
                   ? display.getCurrentRight()
                   : display.getPreviousRight(), right);
  if (PowerState.isInState(Sleeping)) { return; }
  switch (SettingsSM.getCurrentStateId()) {
    case normaldisplay:
    case sethours:
    case setminutes:
      PORTD |= right; 
      break;
    case setseconds:
    case setyear:
    case setmonth:
    case setday:
    case setbrightness:
    case setsleep:
      if (Advance.get_current_state() == down || Decrease.get_current_state() == down || !display.flash) { 
        PORTD |= right; 
      }
  }
  
}

/***************************************************************
 * Have every bulb cycle every digit every time the display is *
 * turned on to prevent cathode poisoning. Multiple methods    *
 * of cycling keep it interesting...                           *
 ***************************************************************/
void cycle_digits() {
  randomSeed(clock.second); // set the seed with the second for "random" values each start
  int display_type = random(3);
  int x;

  display.update(0,0,0, Brightness);
  
  switch (display_type) {
    case 0:  // count up from 0 to 9
      for (int i=0;i<10;i++)
      {
        x = i*10 + i;
        // update all the tubes at once
        display.update( x, x, x, Brightness);
        hold_display(500000);
      }
      break;
    case 1:  // count down from 9 to 0
      for (int i=9;i>=0;i--)
      {
        x = i*10 + i;
        // update all the tubes at once
        display.update( x, x, x, Brightness);
        hold_display(500000);
      }
      break;
    case 2:  // scroll 000000123456789000000 across display
      unsigned long pattern[15] = {0, 1, 12, 123, 1234, 12345, 123456, 234567,
                       345678, 456789, 568790, 678900, 789000, 890000, 900000};
      for (int i=0; i<15; i++) {
        display.update( pattern[i]/10000,
                        (pattern[i] % 10000) / 100,
                        pattern[i] % 100, Brightness);
        hold_display(500000);
      }
      break;
  }
  clock.getTime();
  display.update (clock.hour, clock.minute, clock.second, Brightness);  

}

/*
// todo: update this function
void show_ram(byte addr) {
  byte x=clock.ram[addr];
  byte y=clock.ram[addr+1];
  byte z=clock.ram[addr+2];

  //Serial.println(x);
  outer_step_start_time = micros();
  current_micros = micros();
  while (current_micros - outer_step_start_time < 5000000)
  {
    // update hours
    turn_off_tubes(seconds);
    update_tube_pair(x, hours);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update minutes
    turn_off_tubes(hours);
    update_tube_pair(y, minutes);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update seconds
    turn_off_tubes(minutes);
    update_tube_pair(z, seconds);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }
  }
}
*/
void hold_display(unsigned long microseconds) {
  unsigned long step_start_time;
  unsigned long current_micros;
  // Reset the state machine counters just like in Timer1 interrupt, which isn't enabled yet.
  step_start_time = micros();
  current_micros = micros();
  while(current_micros - step_start_time < microseconds) {
    current_micros = micros();
  }
}

void SettingsButtonPressed() {
  PowerState.resetTick();
  SettingsSM.resetTick();
        
  if (PowerState.isInState(Sleeping)) {
    PowerState.transitionTo(On);
    display.setup_mode = false;
    SettingsSM.transitionTo(NormalDisplay);
    Settings.setWakeUpOverride();
    return;
  } 

  else 

  {
    switch (SettingsSM.getCurrentStateId()) {
      case normaldisplay:
        SettingsSM.transitionTo(SetHours);
        SettingsSM.setTrigger(CancelSettingsMode);
        break;
      case sethours:
        SettingsSM.transitionTo(SetMinutes);
        clock.setTime();
        break;
      case setminutes:
        SettingsSM.transitionTo(SetSeconds);
        clock.setTime();
        break;
      case setseconds:
        SettingsSM.transitionTo(SetYear);
        clock.setTime();
        break;
      case setyear:
        SettingsSM.transitionTo(SetMonth);
        clock.setTime();
        break;
      case setmonth:
        SettingsSM.transitionTo(SetDay);
        clock.setTime();
        break;
      case setday:
        SettingsSM.transitionTo(SetBrightness);
        clock.setTime();
        break;
      case setbrightness:
        SettingsSM.transitionTo(SetSleep);
        clock.setRamAddress(0, Brightness);
        break;
      case setsleep:
        CancelSettingsMode();
        clock.setRamAddress(1, ((PowerState.getMaxTick()/TICKS_PER_MINUTE) >> 8) & 0xFF);
        clock.setRamAddress(2, (PowerState.getMaxTick()/TICKS_PER_MINUTE) & 0xFF);        
    } 

  }
  UpdateSetupDisplay();
  
}

void AdvanceButtonPressed() {
  PowerState.resetTick();
  SettingsSM.resetTick();

  switch (SettingsSM.getCurrentStateId()) {
    case normaldisplay:
      // do nothing
      break;
    case sethours:
      clock.hour++;
      break;
    case setminutes:
      clock.minute++;
      break;
    case setseconds:
      clock.second++;
      break;
    case setyear:
      clock.year++;
      break;
    case setmonth:
      clock.month++;
      break;
    case setday:
      clock.dayOfMonth++;
      clock.dayOfWeek++;
      break;
    case setbrightness:
      Brightness++;
      break;
    case setsleep:
      //unsigned long sleep_minutes = PowerState.getMaxTick()/TICKS_PER_MINUTE;
      PowerState.setMaxTick(PowerState.getMaxTick() + TICKS_PER_MINUTE);
  } 

  ValidateInputs();
  UpdateSetupDisplay();
  
}

void DecreaseButtonPressed() {
  PowerState.resetTick();
  SettingsSM.resetTick();

  switch (SettingsSM.getCurrentStateId()) {
    case normaldisplay:
      // do nothing
      break;
    case sethours:
      clock.hour--;
      break;
    case setminutes:
      clock.minute--;
      break;
    case setseconds:
      clock.second--;
      break;
    case setyear:
      clock.year--;
      break;
    case setmonth:
      clock.month--;
      break;
    case setday:
      clock.dayOfMonth--;
      clock.dayOfWeek--;
      break;
    case setbrightness:
      Brightness--;
      break;
    case setsleep:
      //unsigned long sleep_minutes = PowerState.getMaxTick()/TICKS_PER_MINUTE;
      PowerState.setMaxTick(PowerState.getMaxTick() - TICKS_PER_MINUTE);
  } 

  ValidateInputs();
  UpdateSetupDisplay();
}

void GoToSleep() {
  display.setup_mode = false;
  cycle_digits();
  PowerState.transitionTo(Sleeping);
}

void SetTimerInterrupts() {
  // Configure timer interrupts every 100uS, 250ms

  cli(); // CLear Interrupts, same as noInterrupts();
  TCCR1A = B00000000;    // Timer/Counter1 Control Registers A, B, & C - see datasheet
  TCCR1B = B00001100;    // Bits 0:2->set premult = 256, Bits 3:4->set compare to OCR1A
  TCCR1C = B00000000;    // TCCR1C is only FOC flag which are unused
  TCNT1  = 0;            // Timer/Counter1: Initialize the counter to 0
  OCR1A  = 31249;        // Output Compare Register 1A:   16MHz/(f*premult)-1
                         //   62499: 1Hz, or 1S delay
                         //   31249: 2Hz, 15624: 4Hz
  TCCR2A = B00000010;    // Timer/Counter2 Control Registers A, B, & C - see datasheet
  TCCR2B = B00001100;    // Bits 0:2->set premult = 64, Bits 3:4->set compare to OCR2A
  TCNT2  = 0;            // Timer/Counter2: Initialize the counter to 0
  OCR2A  = 24;           // Output Compare Register 2A:   16MHz/(f*premult)-1
                         //    24: 10000Hz, or 100uS delay
                         //    99 with a premult of 8 gives 20000Hz
  TIMSK2 |= _BV(OCIE2A); // Timer/Counter2 Interrupt Mask Register - Enable

  sei(); // SEt Interrupts, same as Interrupts();
  
}

void ValidateInputs() {
  if (clock.second == 60 ) { clock.second = 0;  clock.minute++; }
  if (clock.second == 255) { clock.second = 59; clock.minute--; }
  if (clock.minute == 60 ) { clock.minute = 0;  clock.hour++; }
  if (clock.minute == 255) { clock.minute = 59; clock.hour--; }
  if (clock.hour == 24 )   { clock.hour = 0;  }
  if (clock.hour == 255)   { clock.hour = 23; }
  // handle various length months for dayOfMonth validation
  switch (clock.month) {
    case 1:  // January
    case 3:  // March
    case 5:  // May
    case 7:  // July
    case 8:  // August
    case 10: // October
    case 12: // December
      if (clock.dayOfMonth == 32 ) { clock.dayOfMonth = 0;  clock.month++; }
      if (clock.dayOfMonth == 255) { clock.dayOfMonth = 31; clock.month--; }
      break;
    case 4:  // April
    case 6:  // June, the best month
    case 9:  // September
    case 11: // Novemeber
      if (clock.dayOfMonth == 31 ) { clock.dayOfMonth = 0;  clock.month++; }
      if (clock.dayOfMonth == 255) { clock.dayOfMonth = 30; clock.month--; }
      break;
    case 2:  // February
      if (clock.year % 4) {
        if (clock.dayOfMonth == 29 ) { clock.dayOfMonth = 0;  clock.month++; }
        if (clock.dayOfMonth == 255) { clock.dayOfMonth = 28; clock.month--; }
      } else {
        if (clock.dayOfMonth == 30 ) { clock.dayOfMonth = 0;  clock.month++; }
        if (clock.dayOfMonth == 255) { clock.dayOfMonth = 28; clock.month--; }
      }
  }
  if (clock.month == 13 ) { clock.month = 0; clock.year++; }
  if (clock.month == 255) { clock.month = 12; clock.year--; }
  if (clock.year == 1999) { clock.year = 2000; }
  if (clock.year == 2100) { clock.year = 2099; }

  if (Brightness == 0) { Brightness = 1; }
  if (Brightness == 9) { Brightness = 8; }

  // todo: set 0 = to permanently on
  if (PowerState.getMaxTick() <= 0) { PowerState.setMaxTick(TICKS_PER_MINUTE); }
  if (PowerState.getMaxTick() > 600*TICKS_PER_MINUTE) { PowerState.setMaxTick(600 * TICKS_PER_MINUTE); }
}

void UpdateSetupDisplay() {
  switch (SettingsSM.getCurrentStateId()) {
    case normaldisplay:
    case sethours:
    case setminutes:
    case setseconds:
      display.update (clock.hour, clock.minute, clock.second, Brightness);
      break;
    case setyear:
      display.update (1, 20, clock.year%100, Brightness );
      break;
    case setmonth:
      display.update (2, 0, clock.month, Brightness );
      break;
    case setday:
      display.update (3, 0, clock.dayOfMonth, Brightness );
      break;
    case setbrightness:
      display.update (4, 0, 9 - Brightness, Brightness );
      break;
    case setsleep:
      unsigned int temp = PowerState.getMaxTick()/TICKS_PER_MINUTE;
      display.update (5, temp / 100, temp % 100, Brightness );
  }
}

void CancelSettingsMode() {
  SettingsSM.transitionTo(NormalDisplay);
  SettingsSM.setTrigger(NULL);      
}
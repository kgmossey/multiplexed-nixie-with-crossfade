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
  btnSettings.init(100, 10000, btnSettingsPressed);
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
  }
  CrossfadeSM.setMaxTick (240);
  LeftTubesOn.setNext(TubesOff_L);
  TubesOff_L.setNext(CenterTubesOn);
  CenterTubesOn.setNext(TubesOff_C);
  TubesOff_C.setNext(RightTubesOn);
  RightTubesOn.setNext(TubesOff_R);
  TubesOff_R.setNext(LeftTubesOn);
  //MultiplexSM.transitionTo(LeftTubesOn);
  //MultiplexSM.transitionNext();

  // Configure timer interrupts every 100uS, 250ms
  cli(); // CLear Interrupts, same as noInterrupts();
  TCCR1A = B00000000;    // Timer/Counter1 Control Registers A, B, & C - see datasheet
  TCCR1B = B00001100;    // Bits 0:2->set premult = 256, Bits 3:4->set compare to OCR1A
  TCCR1C = B00000000;    // TCCR1C is only FOC flag which are unused
  TCNT1  = 0;            // Timer/Counter1: Initialize the counter to 0
  OCR1A  = 31249;        // Output Compare Register 1A:   16MHz/(f*premult)-1
                         //   62499: 1Hz, or 1S delay
                         //   31249: 2Hz, 15624: 4Hz
  TIMSK1 |= _BV(OCIE1A); // Timer/Counter1 Interrupt Mask Register
  TCCR2A = B00000010;    // Timer/Counter2 Control Registers A, B, & C - see datasheet
  TCCR2B = B00001100;    // Bits 0:2->set premult = 64, Bits 3:4->set compare to OCR2A
  TCNT2  = 0;            // Timer/Counter2: Initialize the counter to 0
  OCR2A  = 24;           // Output Compare Register 2A:   16MHz/(f*premult)-1
                         //    24: 10000Hz, or 100uS delay
                         //    99 with a premult of 8 gives 20000Hz
  TIMSK2 |= _BV(OCIE2A); // Timer/Counter2 Interrupt Mask Register

  sei(); // SEt Interrupts, same as Interrupts();
  cycle_digits();
}

void loop() {

  if (TC1IRQ_complete) { // 2Hz interrrupt
    // This getTime() call takes slight over 1ms, needs to be handled outside 
    // interrupt because of the way the wire library works.
    clock.getTime();
    if (DISPLAY_DATE &&
        display_date_step > DISPLAY_DATE_START &&
        display_date_step <= DISPLAY_DATE_END) {
      display.update (clock.year, clock.month, clock.dayOfMonth);
    } else {
      display.update (clock.hour, clock.minute, clock.second);
    }
    display.resetStep();
    MultiplexSM.resetTick();
    CrossfadeSM.resetTick();
    TC1IRQ_complete = false;
  }
  
  if (TC2IRQ_complete) { // 10,000Hz interrupt
    // Everything handled inside interrupt
    // Process needs to take < 100us, currently takes ~20us.
    TC2IRQ_complete = false;
  }

}

/******************************
 * Check time at 2Hz interval *
 ******************************/
ISR(TIMER1_COMPA_vect) {
  display_date_step++;
  if (display_date_step == DISPLAY_DATE_END) { display_date_step = 0; }

  if (display.setup_mode) {
    display.flash = !display.flash;
  } else {
    display.flash = false;
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
      
  multiplexTick = MultiplexSM.getTick();
  xfadeTick = CrossfadeSM.getTick();

  if (multiplexTick == 0 || multiplexTick == 1) {
    MultiplexSM.transitionNext();
  }

  if (xfadeTick == 0) {
    // When the crossfade tick is 0, increment the crossfade pass counter
    display.nextStep();
  }
  if (MultiplexSM.getTick() <= display.getXfadeStep()) {
    CrossfadeSM.transitionTo(Current);
  } else {
    CrossfadeSM.transitionTo(Previous);
  }
  MultiplexSM.update();

// todo: code buttons
/*
  if (SettingsButton == Pressed) {
    buttons |= _BV(SettingsMask);
  } else {
    buttons &= (255 - _BV(SettingsMask));
  }
  if (buttons & _BV(SettingsMask)) {
    btnSettings.set_current_state(down);
  } else {
    btnSettings.set_current_state(up);
  }
  buttons = 0;
*/
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
  // Now we can turn on the tube anode, if it's not in the off cycle of a flashing event
  if (!display.flash) {
    PORTD |= tube_pair;
  }

}

void update_left() {
  // Check cross-fade
#ifdef DEBUG_MODE
  update_right(); 
#else 
  update_tube_pair(CrossfadeSM.isInState(Current)
                   ? display.getCurrentLeft()
                   : display.getPreviousLeft(), left);
#endif
}

void update_center() {
  // Check cross-fade
#ifdef DEBUG_MODE
  update_right(); 
#else 
  update_tube_pair(CrossfadeSM.isInState(Current)
                   ? display.getCurrentCenter()
                   : display.getPreviousCenter(), center); 
#endif
}

void update_right() {
  // Check cross-fade
  update_tube_pair(CrossfadeSM.isInState(Current)
                   ? display.getCurrentRight()
                   : display.getPreviousRight(), right);
}

/***************************************************************
 * Have every bulb cycle every digit every time the display is *
 * turned on to prevent cathode poisoning. Multiple methods    *
 * of cycling keep it interesting...                           *
 ***************************************************************/
void cycle_digits() {
  randomSeed(analogRead(2));  // currently unused pin
  int display_type = random(3);
  int x;

  switch (display_type) {
    case 0:  // count up from 0 to 9
      for (int i=0;i<10;i++)
      {
        x = i*10 + i;
        // update all the tubes at once
        display.update( x, x, x);
        hold_display(250000);
      }
      break;
    case 1:  // count down from 9 to 0
      for (int i=9;i>=0;i--)
      {
        x = i*10 + i;
        // update all the tubes at once
        display.update( x, x, x);
        hold_display(250000);
      }
      break;
    case 2:  // scroll 000000123456789000000 across display
      unsigned long pattern[15] = {0, 1, 12, 123, 1234, 12345, 123456, 234567,
                       345678, 456789, 568790, 678900, 789000, 890000, 900000};
      for (int i=0; i<15; i++) {
        display.update( pattern[i]/10000,
                        (pattern[i] % 10000) / 100,
                        pattern[i] % 100);
        hold_display(200000);
      }
      break;
  }
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
  step_start_time = micros();
  current_micros = micros();
  while(current_micros - step_start_time < microseconds) {
    current_micros = micros();
  }
}

void btnSettingsPressed() {
  display.flash = false;
  display.setup_mode = !display.setup_mode;
}

void btnForwardPressed() {

}

void btnBackPressed() {

}

void btnPowerPressed() {

}

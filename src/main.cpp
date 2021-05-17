// define masks
#define hours           4 // pin 2 mask, bit 2
#define minutes         8 // pin 3 mask, bit 3
#define seconds        16 // pin 4 mask, bit 4
#define SleepButton    (PINB & 0x01) // Pin 14 (D8)
#define SettingsButton (PIND & 0x80) // Pin 13 (D7)
#define PlusButton     (PIND & 0x40) // Pin 12 (D6)
#define MinusButton    (PIND & 0x20) // Pin 11 (D5)
#define SleepMask      1
#define SettingsMask   2
#define PlusMask       3
#define MinusMask      4
#define Pressed        0

#include <Arduino.h>
#include <Wire.h>
#include <Button.h>
#include "DS1307.h"

/*************************
 * Function declarations *
 *************************/
void update_tube_pair (byte value, byte tube_pair);
void cycle_digits();
void show_ram(byte addr);
void turn_off_tubes (byte tube_mask);
void update_hours();
void update_minutes();
void update_seconds();
void hold_display(unsigned long microseconds);
void btnSettingsPressed();
void btnForwardPressed();
void btnBackPressed();
void btnPowerPressed();

/********************
 * Global variables *
 ********************/
DS1307 clock; //define a object of DS1307 class
Button btnSettings;
// Create an index of binary coded decimal values to easily reference two digit numbers
byte bcd[100] = { 0, 128, 32, 160, 16, 144, 48, 176, 64, 192,  // 00 01 02 03 04 05 06 07 08 09
                  8, 136, 40, 168, 24, 152, 56, 184, 72, 200,  // 10 11 12 13 14 15 16 17 18 19
                  2, 130, 34, 162, 18, 146, 50, 178, 66, 194,  // 20 21 22 23 24 25 26 27 28 29
                 10, 138, 42, 170, 26, 154, 58, 186, 74, 202,  // 30 31 32 33 34 35 36 37 38 39
                  1, 129, 33, 161, 17, 145, 49, 177, 65, 193,  // 40 41 42 43 44 45 46 47 48 49
                  9, 137, 41, 169, 25, 153, 57, 185, 73, 201,  // 50 51 52 53 54 55 56 57 58 59
                  3, 131, 35, 163, 19, 147, 51, 179, 67, 195,  // 60 61 62 63 64 65 66 67 68 69
                 11, 139, 43, 171, 27, 155, 59, 187, 75, 203,  // 70 71 72 73 74 75 76 77 78 79
                  4, 132, 36, 164, 20, 148, 52, 180, 68, 196,  // 80 81 82 83 84 85 86 87 88 89
                 12, 140, 44, 172, 28, 156, 60, 188, 76, 204}; // 90 91 92 93 94 95 96 97 98 99
// Note: Nixies should, in general, not be turned off by cathode.  The following codes
// disable the tube by cathode by sending an invalid code to the K155ID1 chip
byte bcd_ones_only[10] = { 15, 143,  47, 175,  31, 159,  63, 191,  79, 207}; // -0 -1 -2 -3 -4 -5 -6 -7 -8 -9
byte bcd_tens_only[10] = {240, 248, 242, 250, 241, 249, 243, 251, 244, 252}; // 0- 1- 2- 3- 4- 5- 6- 7- 8- 9-
bool TC1IRQ_complete = false, TC2IRQ_complete = false; // Timer/Counter X Interrupt Complete
enum power {On, Off, Sleep};
struct display {
  byte hours_tubes;
  byte minutes_tubes;
  byte seconds_tubes;
  byte previous_hours;
  byte previous_minutes;
  byte previous_seconds;
  power power_state;
  bool setup_mode;
  bool flash;
} Display;
unsigned pulse_time = 1000;
byte buttons=0;
byte pass=0;
byte step_counter = 0;
byte premult = 0;

/********************
 * Code starts here *
 ********************/
void setup() {
  // put your setup code here, to run once:
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
  turn_off_tubes(hours + minutes + seconds);
  #ifdef DEBUG_MODE
    Serial.begin(9600);
  #endif
  clock.begin();
  if (!clock.isStarted()) {
    clock.fillByYMD(2021,4,20);     // Y,M,D
    clock.fillByHMS(10,57,00);      // H,M,S
    clock.fillDayOfWeek(TUE);
    clock.setTime();
  }
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
  Display.setup_mode = false;
  Display.power_state = On;
  sei(); // SEt Interrupts, same as Interrupts();
  cycle_digits();
}

void loop() {

  if (TC1IRQ_complete) {
    Display.previous_hours = clock.hour;
    Display.previous_minutes = clock.minute;
    Display.previous_seconds = clock.second;
    clock.getTime();
    pass = 1;
    Display.hours_tubes = clock.hour;
    Display.minutes_tubes = clock.minute;
    Display.seconds_tubes = clock.second;
    premult = 0;
    TC1IRQ_complete = false;
  }

  if (TC2IRQ_complete) {
    // Check cross-fade
    if (step_counter % 10 <= pass) {
      Display.hours_tubes = clock.hour;
      Display.minutes_tubes = clock.minute;
      Display.seconds_tubes = clock.second;
    } else {
      Display.hours_tubes = Display.previous_hours;
      Display.minutes_tubes = Display.previous_minutes;
      Display.seconds_tubes = Display.previous_seconds;
    }
    // Check button states
    if (buttons & _BV(SettingsMask)) {
      btnSettings.set_current_state(down);
    } else {
      btnSettings.set_current_state(up);
    }
    buttons = 0;
    TC2IRQ_complete = false;
  }

}

/******************************
 * Check time at 2Hz interval *
 ******************************/
ISR(TIMER1_COMPA_vect) {
  if (Display.setup_mode) {
    Display.flash = !Display.flash;
  } else {
    Display.flash = false;
  }
  TC1IRQ_complete = true;
}

/*
 * Will fire every 100uS.  See if the state needs to transition,
 * and checks the button states as well.
 */
ISR(TIMER2_COMPA_vect) {
  switch (step_counter) {
    case 0:
      turn_off_tubes(seconds);
      break;
    case 1 ... 9:
      update_hours();
      break;
    case 10:
      turn_off_tubes(hours);
      break;
    case 11 ... 19:
      update_minutes();
      break;
    case 20:
      turn_off_tubes(minutes);
      break;
    case 21 ... 29:
      update_seconds();
      break;
  }
  step_counter++;
  if (step_counter == 30) {
    step_counter = 0;
    if (premult % 8 == 0) {
      if (pass < 9) {pass++;}  // freeze pass at 10
    }
    premult++;
  }

  if (SettingsButton == Pressed) {
    buttons |= _BV(SettingsMask);
  } else {
    buttons &= (255 - _BV(SettingsMask));
  }
  TC2IRQ_complete = true;
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
  if (!Display.flash) {
    PORTD |= tube_pair;
  }

}

void update_hours() {
  update_tube_pair(Display.hours_tubes, hours);
}

void update_minutes() {
  update_tube_pair(Display.minutes_tubes, minutes);
}

void update_seconds() {
  update_tube_pair(Display.seconds_tubes, seconds);
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
        Display.hours_tubes = x;
        Display.minutes_tubes = x;
        Display.seconds_tubes = x;
        hold_display(250000);
      }
      break;
    case 1:  // count down from 9 to 0
      for (int i=9;i>=0;i--)
      {
        x = i*10 + i;
        // update all the tubes at once
        Display.hours_tubes = x;
        Display.minutes_tubes = x;
        Display.seconds_tubes = x;
        hold_display(250000);
      }
      break;
    case 2:  // scroll 000000123456789000000 across display
      unsigned long pattern[15] = {0, 1, 12, 123, 1234, 12345, 123456, 234567,
                       345678, 456789, 568790, 678900, 789000, 890000, 900000};
      for (int i=0; i<15; i++) {
        Display.hours_tubes = pattern[i]/10000;
        Display.minutes_tubes = (pattern[i] % 10000) / 100;
        Display.seconds_tubes = pattern[i] % 100;
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
  Display.flash = false;
  Display.setup_mode = !Display.setup_mode;
}

void btnForwardPressed() {

}

void btnBackPressed() {

}

void btnPowerPressed() {

}

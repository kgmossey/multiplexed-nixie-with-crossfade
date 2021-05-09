// define pins
#define hours_pin      2 // PD2
#define minutes_pin    3 // PD3
#define seconds_pin    4 // PD4
#define A_ones        16 // PC2
#define B_ones        14 // PC0
#define C_ones        13 // PB5
#define D_ones        15 // PC1
#define A_tens        12 // PB4
#define B_tens        10 // PB2
#define C_tens         9 // PB1
#define D_tens        11 // PB3
// define masks
#define hours          4 // pin 2 mask, bit 2
#define minutes        8 // pin 3 mask, bit 3
#define seconds       16 // pin 4 mask, bit 4

#include <Arduino.h>
#include <Wire.h>
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

/********************
 * Global variables *
 ********************/
DS1307 clock; //define a object of DS1307 class
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
struct display {
  byte hours_tubes;
  byte minutes_tubes;
  byte seconds_tubes;
} Display;
unsigned long inner_step_start_time;
unsigned long outer_step_start_time;
unsigned long current_micros;
unsigned pulse_time = 1000;
bool getUpdatedTime = false;

/********************
 * Code starts here *
 ********************/
void setup() {
  // put your setup code here, to run once:
  pinMode(A_tens,      OUTPUT);
  pinMode(B_tens,      OUTPUT);
  pinMode(C_tens,      OUTPUT);
  pinMode(D_tens,      OUTPUT);
  pinMode(A_ones,      OUTPUT);
  pinMode(B_ones,      OUTPUT);
  pinMode(C_ones,      OUTPUT);
  pinMode(D_ones,      OUTPUT);
  pinMode(hours_pin,   OUTPUT);
  pinMode(minutes_pin, OUTPUT);
  pinMode(seconds_pin, OUTPUT);
  turn_off_tubes(hours + minutes + seconds);
  Serial.begin(9600);
  clock.begin();
  if (!clock.isStarted()) {
    clock.fillByYMD(2021,4,20);     // Y,M,D
    clock.fillByHMS(10,57,00);      // H,M,S
    clock.fillDayOfWeek(TUE);
    clock.setTime();
  }
  // Configure timer interrupts every 100uS
  cli(); // CLear Interrupts, same as noInterrupts();
  TCCR1A = B00000000;   // Timer/Counter1 Control Registers A, B, & C - see datasheet
  TCCR1B = B00001011;   // Bits 0:2->set premult = 64, Bits 3:4->set compare to OCR1A
  TCCR1C = B00000000;   //
  TCNT1  = 0;           // Timer/Counter1: Initialize the counter to 0
  OCR1A  = 124;         // Output Compare Register 1A, 1B:   16MHz/(f*premult)-1
  OCR1B  = 249;         //    24: 10000Hz, or 100uS delay
                        //   124: 2000Hz, or 500uS
                        //   249: 1000Hz, or 1000uS
                        //   499: 500Hz, or 2000uS   <- values > 255 are why you
                        //   749: 333 1/3Hz, or 3000uS           need to use the
                        //   999: 250 Hz, or 4000uS              16 bit Timer1
  TIMSK1 |= _BV(OCIE1A);// Timer/Counter1 Interrupt Mask Register
  sei(); // SEt Interrupts, same as Interrupts();
  //cycle_digits();
}

void loop() {
  if (getUpdatedTime) {
    clock.getTime();
    Display.hours_tubes = clock.hour;
    Display.minutes_tubes = clock.minute;
    Display.seconds_tubes = clock.second;
    getUpdatedTime = false;
  }
}

/*
 * Will fire every 100uS.  See if the state needs to transition,
 * and checks the button states as well.
 */
ISR(TIMER1_COMPA_vect) {
  static byte step_counter = 0;
  switch (step_counter) {
    case 0:
      getUpdatedTime = true;
      turn_off_tubes(seconds);
      break;
    case 1:
      update_hours();
      break;
    case 10:
      turn_off_tubes(hours);
      break;
    case 11:
      update_minutes();
      break;
    case 20:
      turn_off_tubes(minutes);
      break;
    case 21:
      update_seconds();
      break;
  }
  step_counter++;
  if (step_counter == 30) { step_counter = 0; }
}

void turn_off_tubes(byte tube_mask) {
/* examples to play with later
  PORTC |= _BV(0);  // Set bit 0 only.
  PORTC &= ~(_BV(1));  // Clear bit 1 only.
  PORTC ^= _BV(7);  // Toggle bit 7 only.
*/
  PORTD &= 255 - tube_mask;
  //delayMicroseconds(100);   // give time for tubes to discharge
}

void update_tube_pair (byte value, byte tube_pair){

  // Turning off the anode even though it should already be off to prevent
  // cathode switching in the case of cross-fading
  turn_off_tubes(tube_pair);
  PORTB = (bcd[value] << 1) & B00111110;
  PORTC = (bcd[value] >> 5) & B00000111;
  // Now we can turn on the tube anode
  PORTD |= tube_pair;

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

void cycle_digits() {
  randomSeed(analogRead(2));  // currently unused pin
  int display_type = random(2);
  int x;
  bool done_looping = false;
  int increment = 0;

  switch (display_type) {
    case 0:
      for (int i=0;i<10;i++)
      {
        x = i*10 + i;
        // update all the tubes at once
        Display.hours_tubes = x;
        Display.minutes_tubes = x;
        Display.seconds_tubes = x;
        /*while (!done_looping) {
          if (getUpdatedTime) {
            increment++;
            getUpdatedTime = false;
            if (increment > 1000) {
              done_looping = true;
            }
          }
        }*/
        //turn_off_tubes(hours + minutes + seconds);
        //update_tube_pair(x, hours + minutes + seconds);
        inner_step_start_time = micros();
        current_micros = micros();
        while(current_micros - inner_step_start_time < 250000) {
          current_micros = micros();
        }
      }
      break;
    case 1:
      for (int i=9;i>=0;i--)
      {
        x = i*10 + i;
        // update all the tubes at once
        Display.hours_tubes = x;
        Display.minutes_tubes = x;
        Display.seconds_tubes = x;
        /*while (!done_looping) {
          if (getUpdatedTime) {
            increment++;
            getUpdatedTime = false;
            if (increment == 1000) {
              done_looping = true;
            }
          }
        }*/
        //turn_off_tubes(hours + minutes + seconds);
        //update_tube_pair(x, hours + minutes + seconds);
        inner_step_start_time = micros();
        current_micros = micros();
        while(current_micros - inner_step_start_time < 250000) {
          current_micros = micros();
        }
      }
      break;
    case 2:
      break;
      // scroll 000000123456789000000 across display
  }
}

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

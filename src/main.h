#ifndef NIXIE_H
#define NIXIE_H

/************
 * Includes *
 ************/
#include <Arduino.h>
#include <Wire.h>
#include <Button.h>
#include "DS1307.h"
#include "StateMachine.h"
#include "display.h"

// define masks
#define left           8             // pin 3 mask, bit 3
#define center         4             // pin 2 mask, bit 2
#define right          16            // pin 4 mask, bit 4
#define none           0             // no pins
#define SleepButton    (PINB & 0x01) // Pin 14 (D8)
#define SettingsButton (PIND & 0x80) // Pin 13 (D7)
#define PlusButton     (PIND & 0x40) // Pin 12 (D6)
#define MinusButton    (PIND & 0x20) // Pin 11 (D5)
#define SleepMask      1
#define SettingsMask   2
#define PlusMask       3
#define MinusMask      4
#define Pressed        0
#define BrightnessHi   1
#define BrightnessMed  4
#define BrightnessLow  8

//#define DEBUG_MODE
#define DISPLAY_DATE        0  // 1 is show date every x cycles
#define DISPLAY_DATE_START 11  // If DISPLAY_DATE is 1, show date between
#define DISPLAY_DATE_END   16  // the start and end values

/*************************
 * Function declarations *
 *************************/
void update_tube_pair (byte value, byte tube_pair);
void cycle_digits();
void show_ram(byte addr);
void turn_off_tubes (byte tube_mask);
void turn_off_all_tubes();
void update_left();
void update_center();
void update_right();
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

enum CrossfadeStates {
  PreviousValue,
  CurrentValue
};

State LeftTubesOn (left, update_left);
State TubesOff_L (none, turn_off_all_tubes);
State CenterTubesOn (center, update_center);
State TubesOff_C (none, turn_off_all_tubes);
State RightTubesOn (right, update_right);
State TubesOff_R (none, turn_off_all_tubes);
FSM MultiplexSM = FSM(TubesOff_L);

State Previous(PreviousValue);
State Current(CurrentValue);
FSM CrossfadeSM = FSM(Current);

Display display(10);
byte Brightness = BrightnessHi;

byte buttons = 0;
byte display_date_step = 0;

#endif
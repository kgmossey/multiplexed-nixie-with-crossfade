//#define DEBUG_MODE
#ifndef NIXIE_H
#define NIXIE_H

/************
 * Includes *
 ************/
#include <Arduino.h>
#include <Wire.h>
#include <Button.h>
#include "DS1307.h"
#include "FiniteStateMachine.h"
#include "display.h"

// define masks
#define left            8             // pin 3 mask, bit 3
#define center          4             // pin 2 mask, bit 2
#define right           16            // pin 4 mask, bit 4
#define none            0             // no pins
//#define SettingsButton     (PINB & 0x01) // Pin 14 (D8)
#define SettingsButton  (PIND & 0x80) // Pin 13 (D7)
#define AdvanceButton   (PIND & 0x40) // Pin 12 (D6)
#define DecreaseButton  (PIND & 0x20) // Pin 11 (D5)
//#define Future1       (PIND & 0x20) // Pin 11 (D5)
//#define Future2       (PIND & 0x20) // Pin  3 (D1)
//#define Future3       (PIND & 0x20) // Pin  2 (D0)
//#define Future4       (PIND & 0x20) // Pin 26 (A3)
#define SettingsMask    2
#define PlusMask        3
#define MinusMask       4
#define Pressed         0
#define BrightnessHi    1
#define BrightnessMed   4
#define BrightnessLow   8

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
void SettingsButtonPressed();
void AdvanceButtonPressed();
void DecreaseButtonPressed();
void GoToSleep();
void SetTimerInterrupts();
void SetTimerInterrupts();
void ValidateInputs();
void UpdateSetupDisplay();
void CancelSettingsMode();
void CathodeProtectionRoutine();
void CathodeProtectionTrigger();
void WakeupRoutine();

/********************
 * Global variables *
 ********************/
DS1307 clock; //define a object of DS1307 class
unsigned int button_repeat_pattern[24] = { 15000, 8000, 8000, 8000, 8000, 8000, 
                                           4000, 4000, 4000, 4000, 4000, 4000, 4000, 
                                           2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 
                                           1000 };
//unsigned int test_pattern [3] = { 1000, 500, 250};
Button Settings (SettingsButtonPressed);
Button Advance (AdvanceButtonPressed, button_repeat_pattern, sizeof(button_repeat_pattern)/sizeof(button_repeat_pattern[0]));
Button Decrease (DecreaseButtonPressed, button_repeat_pattern, sizeof(button_repeat_pattern)/sizeof(button_repeat_pattern[0]));
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

const unsigned long TICKS_PER_MINUTE = 600000; // 60 * 10,000Hz
const unsigned long TICKS_PER_SECOND = 10000;  // defined by IRQ2
const unsigned long WAKEUP_TICKS = 50;
unsigned long SLEEP_MAX_TICK = 0;

// define memory addresses
const byte addrBrightness = 0;
const byte addrSleepHi = 1;
const byte addrSleepLow = 2;
const byte addrAPtubes = 3;
const byte addrWake = 4;

// To keep track of whether a setting has been changed and needs to be saved
bool Dirty = false;

State LeftTubesOn (left, update_left);
State TubesOff_L (none, turn_off_all_tubes);
State CenterTubesOn (center, update_center);
State TubesOff_C (none, turn_off_all_tubes);
State RightTubesOn (right, update_right);
State TubesOff_R (none, turn_off_all_tubes);
FSM MultiplexSM = FSM(TubesOff_L);

enum PowerStates {
  psSleeping,
  psWakeUp,
  psOn,
  psCathodeProtection // this is to be used for running a once daily anti-poisoning routine 
};

State Sleeping(psSleeping); 
State WakeUp(psWakeUp);
State On(psOn); 
State CathodeProtection(psCathodeProtection, CathodeProtectionRoutine);
FSM PowerState(On);

enum CrossfadeStates {
  PreviousValue,
  CurrentValue
};

State Previous(PreviousValue);
State Current(CurrentValue);
FSM CrossfadeSM = FSM(Current);

enum SettingsStates {
  normaldisplay,
  sethours,
  setminutes,
  setseconds,
  setyear,
  setmonth,
  setday,
  setbrightness,
  setsleep,
  setwake
};

State NormalDisplay(normaldisplay);
State SetHours(sethours);
State SetMinutes(setminutes);
State SetSeconds(setseconds);
State SetYear(setyear); // (Range of chip)
State SetMonth(setmonth);
State SetDay(setday);// (1-31, depending on Month)
State SetBrightness(setbrightness); // 1-8
State SetSleep(setsleep);
State SetWake(setwake);
FSM SettingsSM = FSM(NormalDisplay);

Display display(10);
byte Brightness = BrightnessHi;
byte WakeupTime = 10; // Multiply by 10 

byte buttons = 0;
byte display_date_step = 0;

#endif

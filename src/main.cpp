// define pins
#define hours_pin         2
#define minutes_pin       3
#define seconds_pin       4
#define hours             4 // pin 2 mask
#define hours_off       251 // pin 2
#define minutes           8 // pin 3 mask
#define minutes_off     247 // pin 3
#define seconds          16 // pin 4 mask
#define seconds_off     239 // pin 4
#define tubes_off_mask  227 // B11100011
#define tubes_on_mask    28 // B00011100
#define A_ones   16 // PC2
#define B_ones   14 // PC0
#define C_ones   13 // PB5
#define D_ones   15 // PC1
#define A_tens   12 // PB4
#define B_tens   10 // PB2
#define C_tens   9  // PB1
#define D_tens   11 // PB3

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

unsigned long inner_step_start_time;
unsigned long outer_step_start_time;
unsigned long current_micros;
unsigned pulse_time = 1000;

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
  Serial.begin(9600);
  clock.begin();
  if (!clock.isStarted()) {
    clock.fillByYMD(2021,4,20);     // Y,M,D
    clock.fillByHMS(10,57,00);      // H,M,S
    clock.fillDayOfWeek(TUE);
    clock.setTime();
  }
  cycle_digits();
}

void loop() {
  outer_step_start_time = micros();
  current_micros = micros();
  clock.getTime();
  while (current_micros - outer_step_start_time < 200000)
  {
    // update hours //
    // Turn off seconds (which was previously lit)
    turn_off_tubes(seconds);
    // Update digits, turn on the hours for the set step time
    update_tube_pair(clock.hour, hours);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update minutes //
    // Turn off hours (which was previously lit)
    turn_off_tubes(hours);
    // Update digits, turn on the minutes for the set step time
    update_tube_pair(clock.minute, minutes);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update seconds
    // Turn off minutes (which was previously lit)
    turn_off_tubes(minutes);
    // Update digits, turn on the seconds for the set step time
    update_tube_pair(clock.second, seconds);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }
  }
}

void turn_off_tubes(byte tube_mask) {
  PORTD = PORTD & (255 - tube_mask);
  delay(100);   // give time for tubes to discharge
}

void update_tube_pair (byte value, byte tube_pair){

  PORTB = (bcd[value] << 1) & B00111110;
  PORTC = (bcd[value] >> 5) & B00000111;
  PORTD = PORTD | tube_pair;

}

void cycle_digits() {
  byte x;
  for (byte i=0;i<10;i++)
  {
    x = i*10 + i;
    // update all the tubes at once
    turn_off_tubes(hours + minutes + seconds);
    update_tube_pair(x, hours + minutes + seconds);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < 250000) {
      current_micros = micros();
    }
  }
  for (byte i=9;i>=0;i--)
  {
    x = i*10 + i;
    // update all the tubes at once
    turn_off_tubes(hours + minutes + seconds);
    update_tube_pair(x, hours + minutes + seconds);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < 250000) {
      current_micros = micros();
    }
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

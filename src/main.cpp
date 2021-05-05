// define pins
#define hours    2
#define minutes  3
#define seconds  4
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

DS1307 clock; //define a object of DS1307 class
// Create an index if binary coded decimal values to easily reference
byte bcd[100] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  // 00 01 02 03 04 05 06 07 08 09
                 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  // 10 11 12 13 14 15 16 17 18 19
                 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,  // 20 21 22 23 24 25 26 27 28 29
                 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,  // 30 31 32 33 34 35 36 37 38 39
                 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,  // 40 41 42 43 44 45 46 47 48 49
                 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,  // 50 51 52 53 54 55 56 57 58 59
                 96, 97, 98, 99,100,101,102,103,104,105,  // 60 61 62 63 64 65 66 67 68 69
                112,113,114,115,116,117,118,119,120,121,  // 70 71 72 73 74 75 76 77 78 79
                128,129,130,131,132,133,134,135,136,137,  // 80 81 82 83 84 85 86 87 88 89
                144,145,146,147,148,149,150,151,152,153}; // 90 91 92 93 94 95 96 97 98 99
byte tubelist[6][4] = {
  {A_ones, B_ones, C_ones, D_ones}, // seconds ones place
  {A_tens, B_tens, C_tens, D_tens}, // seconds tens place
  {A_ones, B_ones, C_ones, D_ones}, // minutes ones place
  {A_tens, B_tens, C_tens, D_tens}, // minutes tens place
  {A_ones, B_ones, C_ones, D_ones}, // hours ones place
  {A_tens, B_tens, C_tens, D_tens}  // hours tens place
};
enum tubeID {
  seconds_ones, seconds_tens, minutes_ones, minutes_tens, hours_ones, hours_tens
};
unsigned long inner_step_start_time;
unsigned long outer_step_start_time;
unsigned long current_micros;
unsigned pulse_time = 1000;

/*************************
 * Function declarations *
 *************************/
void update_tube_pair (byte value);
void update_tube (byte tube, byte value);
void cycle_digits();
void show_ram(byte addr);
void fadebetween(byte from, byte to);

/********************
 * Code starts here *
 ********************/
void setup() {
  // put your setup code here, to run once:
  pinMode(A_tens,  OUTPUT);
  pinMode(B_tens,  OUTPUT);
  pinMode(C_tens,  OUTPUT);
  pinMode(D_tens,  OUTPUT);
  pinMode(A_ones,  OUTPUT);
  pinMode(B_ones,  OUTPUT);
  pinMode(C_ones,  OUTPUT);
  pinMode(D_ones,  OUTPUT);
  pinMode(hours,   OUTPUT);
  pinMode(minutes, OUTPUT);
  pinMode(seconds, OUTPUT);
  Serial.begin(9600);
  clock.begin();
  if (!clock.isStarted()) {
    clock.fillByYMD(2021,4,20);     // Y,M,D
    clock.fillByHMS(10,57,00);      // H,M,S
    clock.fillDayOfWeek(TUE);
    clock.setTime();
  }
  //byte myram[56]={56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};
  //memcpy(clock.ram, myram, 56);
  //for (int i = 0; i<=55;i++){Serial.println(clock.ram[i]);}
//  Serial.println();
  //for (int i = 0; i<=55;++i){Serial.println(clock.ram[i]);}
  cycle_digits();
}

void loop() {
  outer_step_start_time = micros();
  current_micros = micros();
  clock.getTime();
  while (current_micros - outer_step_start_time < 200000)
  {
    // update hours
    digitalWrite(seconds, LOW);
    update_tube_pair(bcd[clock.hour]);
    digitalWrite(hours, HIGH);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update minutes
    digitalWrite(hours, LOW);
    update_tube_pair(bcd[clock.minute]);
    digitalWrite(minutes, HIGH);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update seconds
    digitalWrite(minutes, LOW);
    update_tube_pair(bcd[clock.second]);
    digitalWrite(seconds, HIGH);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }
  }
}

void update_tube_pair (byte value){

  byte a2 =   1 & value ? HIGH : LOW;
  byte b2 =   2 & value ? HIGH : LOW;
  byte c2 =   4 & value ? HIGH : LOW;
  byte d2 =   8 & value ? HIGH : LOW;
  byte a1 =  16 & value ? HIGH : LOW;
  byte b1 =  32 & value ? HIGH : LOW;
  byte c1 =  64 & value ? HIGH : LOW;
  byte d1 = 128 & value ? HIGH : LOW;

  digitalWrite(A_tens, a1);
  digitalWrite(B_tens, b1);
  digitalWrite(C_tens, c1);
  digitalWrite(D_tens, d1);
  digitalWrite(A_ones, a2);
  digitalWrite(B_ones, b2);
  digitalWrite(C_ones, c2);
  digitalWrite(D_ones, d2);

}

void update_tube (byte tube, byte value)
{
/*
  // Dim old digit
  for (byte brightness = 255; brightness > 55; brightness--) {
    analogWrite(pw, brightness);
    delay(1);
  }
*/
  // full turn off digit
  byte a = 1 & value ? HIGH : LOW;
  byte b = 2 & value ? HIGH : LOW;
  byte c = 4 & value ? HIGH : LOW;
  byte d = 8 & value ? HIGH : LOW;
  // write new digit
  digitalWrite(tubelist[tube][0], a);
  digitalWrite(tubelist[tube][1], b);
  digitalWrite(tubelist[tube][2], c);
  digitalWrite(tubelist[tube][3], d);
/*
  // brighten new digit
  for (byte brightness = 55; brightness < 255; brightness++) {
    analogWrite(pw, brightness);
    delay(1);
  }
*/

if (0) {fadebetween(9,0);}
}


void fadebetween(byte from, byte to) {

  unsigned step_start_time;
  unsigned current_micros;
  byte old_brightness, new_brightness;
/*
  byte a1 = 1 & from ? HIGH : LOW;     byte a2 = 1 & to ? HIGH : LOW;
  byte b1 = 2 & from ? HIGH : LOW;     byte b2 = 2 & to ? HIGH : LOW;
  byte c1 = 4 & from ? HIGH : LOW;     byte c2 = 4 & to ? HIGH : LOW;
  byte d1 = 8 & from ? HIGH : LOW;     byte d2 = 8 & to ? HIGH : LOW;
*/
  new_brightness = 0;
  for (old_brightness = 255; old_brightness > 0; old_brightness--) {
    step_start_time = micros();
    current_micros = micros();
    while(current_micros - step_start_time < 2000) {
      update_tube (0, to);
      analogWrite(hours, old_brightness);

      current_micros++;
      current_micros = micros();

      update_tube (0, from);
      analogWrite(hours, new_brightness);

      new_brightness++;
      current_micros = micros();
    }
  }
  digitalWrite(hours, HIGH);
}

void cycle_digits() {
  byte x;
  for (byte i=0;i<10;i++)
  {
    x = (i<<4)+i;
    //Serial.println(x);
    outer_step_start_time = micros();
    current_micros = micros();
    while (current_micros - outer_step_start_time < 250000)
    {
      // update hours
      digitalWrite(seconds, LOW);
      update_tube_pair(x);
      digitalWrite(hours, HIGH);
      inner_step_start_time = micros();
      current_micros = micros();
      while(current_micros - inner_step_start_time < pulse_time) {
        current_micros = micros();
      }

      // update minutes
      digitalWrite(hours, LOW);
      update_tube_pair(x);
      digitalWrite(minutes, HIGH);
      inner_step_start_time = micros();
      current_micros = micros();
      while(current_micros - inner_step_start_time < pulse_time) {
        current_micros = micros();
      }

      // update seconds
      digitalWrite(minutes, LOW);
      update_tube_pair(x);
      digitalWrite(seconds, HIGH);
      inner_step_start_time = micros();
      current_micros = micros();
      while(current_micros - inner_step_start_time < pulse_time-200) {
        current_micros = micros();
      }
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
    digitalWrite(seconds, LOW);
    update_tube_pair(bcd[x]);
    digitalWrite(hours, HIGH);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update minutes
    digitalWrite(hours, LOW);
    update_tube_pair(bcd[y]);
    digitalWrite(minutes, HIGH);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }

    // update seconds
    digitalWrite(minutes, LOW);
    update_tube_pair(bcd[z]);
    digitalWrite(seconds, HIGH);
    inner_step_start_time = micros();
    current_micros = micros();
    while(current_micros - inner_step_start_time < pulse_time) {
      current_micros = micros();
    }
  }


}

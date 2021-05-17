# multiplexed-nixie-with-crossfade
Arduino based multiplexed nixie clock, with configurable crossfade between digit transitions.

The general config of the clock is as follows: there needs to be 3 pins of output to toggle the anodes on/off for each of the hours, minutes, and seconds tube pairs.  There are 4 pins x2 of output to each of the K155ID1 BCD chips.  There are 4 pins of input utilizing the internal pullup resistor for each of the settings buttons.

Unfortunately the Russian driver chip (K155ID1) does not use the bits in order with the input pins.  To keep the circuit board layout simple I mapped the atmega328p pins directly to the K155ID1 inputs.  I then created a funky BCD array to convert each digit to it's appropriate output pins.  I then bit shift and mask these values to write directly to the ports for output.  This ends up being between 20-40x faster than using the Arduino digitalWrite() function.

The anode driving is done with a transistor pair high side switch using M42 (HV NPN) and M92 (HV PNP) transistors capable of running at 50 MHz, which is significantly faster than the 16 MHz of the Arduino.

It uses Timer1 and Timer2 interrupts, and requires the external DS1307 library (todo: add library link) as well as the arduino library wire.h for communication with the real time clock.  I've modified the library slightly to read/write data to the 56 bytes of nvram, to store settings when the power is off.  I leave Timer0 alone since other libraries are using the micros() or millis() functions.  

Crossfading digits is done by slowly increasing the on time of the new digit and slowly decreasing the on time of the old digit until the new digit is completely on.  Each tube pair is on for 900us (microseconds) and then off for 2100us, in 100us increments.  So the on time goes in the order of 0, 100, 200, ... 800, 900 us, over the course of several cycles.  All the tubes are off for 100us every cycle to prevent bleeding of the next pairs values into the tube that has just turned off.  The nixie tube is the slowest piece of equipment in the whole clock.

To maximize tube life, the clock will be designed to automatically sleep, as of this writing I haven't implemented this code yet.

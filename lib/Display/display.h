#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

class Tubes {
  public:
    Tubes ();
    Tubes (byte Left, byte Center, byte Right);

    byte getLeft();
    byte getCenter();
    byte getRight();
    void getAll(byte& Left, byte& Center, byte& Right);

    void setLeft(byte Left);
    void setCenter(byte Center);
    void setRight(byte Right);
    void setAll(byte Left, byte Center, byte Right);

  private:
    byte left;
    byte center;
    byte right;

};

class Display {
  public:
    bool flash;
    bool setup_mode;

    byte getPreviousLeft();
    byte getPreviousCenter();
    byte getPreviousRight();
    byte getCurrentLeft();
    byte getCurrentCenter();
    byte getCurrentRight();
    void update(byte Left, byte Center, byte Right);
    void nextStep();
    byte getXfadeStep();
    void resetStep();

    Display();
    Display(byte xfadeSteps);

  private:
    Tubes previous;
    Tubes current;
    byte xfadeStep;
    byte maxXfade;

};

#endif

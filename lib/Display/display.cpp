#include "display.h"

Tubes::Tubes (byte Left, byte Center, byte Right)
{
  left = Left;
  center = Center;
  right = Right;
}

Tubes::Tubes()
{
  left = 0;
  center = 0;
  right = 0;
}

byte Tubes::getLeft()
{
  return left;
}

byte Tubes::getCenter()
{
  return center;
}

byte Tubes::getRight()
{
  return right;
}

void Tubes::getAll(byte& Left, byte& Center, byte& Right)
{
  Left = left;
  Center = center;
  Right = right;
}

void Tubes::setLeft(byte Left)
{
  if (Left < 0) { left = 99; }
  else if (Left > 99) { left = 0; }
  else { left = Left; }
}

void Tubes::setCenter(byte Center)
{
  if (Center < 0) { center = 99; }
  else if (Center > 99) { center = 0; }
  else { center = Center; }
}

void Tubes::setRight(byte Right)
{
  if (Right < 0) { right = 99; }
  else if (Right > 99) { right = 0; }
  else { right = Right; }
}

void Tubes::setAll(byte Left, byte Center, byte Right)
{
  setLeft(Left);
  setCenter(Center);
  setRight(Right);
}

/******************************************************/

Display::Display()
{
  maxXfade = 10;
}

Display::Display(byte xfadeSteps)
{
  maxXfade = xfadeSteps;
}

void Display::update(byte Left, byte Center, byte Right)
{
  previous = current;
  TriggerSet = true;
  current.setAll(Left, Center, Right);
  xfadeStep = 0;
}

byte Display::getPreviousLeft()
{
  return previous.getLeft();
}

byte Display::getPreviousCenter()
{
  return previous.getCenter();
}

byte Display::getPreviousRight()
{
  return previous.getRight();
}

byte Display::getCurrentLeft()
{
  return current.getLeft();
}

byte Display::getCurrentCenter()
{
  return current.getCenter();
}

byte Display::getCurrentRight()
{
  return current.getRight();
}

void Display::nextStep()
{
//Serial.println("x");
  if (xfadeStep < maxXfade) {
    xfadeStep++;
  } else {
    AllowTransition = false;
  }
  // old way of doing it
  //if (xfadeStep > maxXfade) { xfadeStep = maxXfade; }
}

byte Display::getXfadeStep()
{
  return xfadeStep;
}

void Display::resetStep()
{
  xfadeStep = 0;
  AllowTransition = true;
  TriggerSet = false;
}

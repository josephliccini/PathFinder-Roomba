#include <Roomba.h>

Roomba roomba(&Serial1, Roomba::Baud19200);
int ledPin =  13;

void setup()
{    
    pinMode(ledPin, OUTPUT);  
    roomba.start();

    roomba.fullMode();
}

void loop()
{
  roomba.leds(ROOMBA_MASK_LED_ADVANCE, 255, 255);

  roomba.drive(0, 0);

  delay(700);
  digitalWrite(ledPin, HIGH);
  
  roomba.leds(ROOMBA_MASK_LED_ADVANCE, 0, 255);

  roomba.drive(0, 0);
  
  delay(700);
  digitalWrite(ledPin, LOW);
}

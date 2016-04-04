#include <Roomba.h>

// Roomba roomba(&Serial1, Roomba::Baud19200);
int ledPin =  13;

void setup()
{    
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);  
    // roomba.start();

    // roomba.fullMode();
}

void loop()
{
  // roomba.leds(ROOMBA_MASK_LED_ADVANCE, 255, 255);
  system("rfkill unblock bluetooth");
  system("hciconfig hci0 down");
  system("hciconfig hci0 up");
  system("node /home/root/basic.js > /dev/ttyGS0 2>&1 &");
  
  // roomba.drive(100, 0);
  digitalWrite(ledPin, HIGH);

  delay(9000);
  // roomba.drive(100, 0);
  Serial.print("What's Up Bitches");

  // delay(700);
  digitalWrite(ledPin, HIGH);
  
  // roomba.leds(ROOMBA_MASK_LED_ADVANCE, 0, 255);

  // roomba.drive(-100, 0);
  
  delay(700);
  digitalWrite(ledPin, LOW);
  delay(700);
}

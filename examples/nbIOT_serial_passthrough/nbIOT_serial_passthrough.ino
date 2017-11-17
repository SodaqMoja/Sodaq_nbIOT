#include "Arduino.h"

#if defined(ARDUINO_AVR_LEONARDO)
#define DEBUG_STREAM Serial 
#define MODEM_STREAM Serial1
#define powerPin 7 

#elif defined(ARDUINO_SODAQ_EXPLORER)
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial
#define powerPin 7 

#elif defined(ARDUINO_SAM_ZERO)
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define powerPin 7 

#else
#error "Please select a Sodaq ExpLoRer, Arduino Leonardo, Arduino M0 (Crowduino M0) or add your board."
#endif

void setup() 
{
  // Turn the nb-iot module on
  pinMode(powerPin, OUTPUT); 
  digitalWrite(powerPin, HIGH);

  // Start communication
  DEBUG_STREAM.begin(9600);
  MODEM_STREAM.begin(9600);
}

// Forward every message to the other serial
void loop() 
{
  while (DEBUG_STREAM.available())
  {
	uint8_t c = DEBUG_STREAM.read();
	DEBUG_STREAM.write(c);
    MODEM_STREAM.write(c);
  }

  while (MODEM_STREAM.available())
  {     
    DEBUG_STREAM.write(MODEM_STREAM.read());
  }
}

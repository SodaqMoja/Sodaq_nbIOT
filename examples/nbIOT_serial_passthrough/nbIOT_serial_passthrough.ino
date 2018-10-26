#include <Arduino.h>

// #define R4XX // Uncomment when you use the ublox R4XX module

#if defined(ARDUINO_AVR_LEONARDO)
/* Arduino Leonardo + SODAQ NB-IoT Shield */
#define DEBUG_STREAM Serial 
#define MODEM_STREAM Serial1
#define powerPin 7 

#elif defined(ARDUINO_SODAQ_EXPLORER)
/* SODAQ Explorer + SODAQ NB-IoT Shield */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial
#define powerPin 7 

#elif defined(ARDUINO_SAM_ZERO)
/* Arduino Zero / M0 + SODAQ NB-IoT Shield */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define powerPin 7 

#elif defined(ARDUINO_SODAQ_AUTONOMO)
/* SODAQ AUTONOMO + SODAQ NB-IoT Bee */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define powerPin BEE_VCC
#define enablePin BEEDTR

#elif defined(ARDUINO_AVR_SODAQ_MBILI)
/* SODAQ MBILI + SODAQ NB-IoT Bee */
#define DEBUG_STREAM Serial
#define MODEM_STREAM Serial1
#define enablePin BEEDTR

#elif defined(ARDUINO_SODAQ_SARA)
/* SODAQ SARA */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define powerPin SARA_ENABLE
#define enablePin SARA_TX_ENABLE
#define voltagePin SARA_R4XX_TOGGLE

#elif defined(ARDUINO_SODAQ_SFF)
/* SODAQ SARA SFF */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define powerPin SARA_ENABLE
#define voltagePin SARA_R4XX_TOGGLE

#else
#error "Please use one of the listed boards or add your board."
#endif

#if defined(R4XX)
unsigned long baud = 115200;  //start at 115200 allow the USB port to change the Baudrate
#else 
unsigned long baud = 9600;  //start at 9600 allow the USB port to change the Baudrate
#endif

void setup() 
{
#ifdef powerPin
    // Put voltage on the nb-iot module
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
#endif

#ifdef R4XX
    // Switch module voltage
    pinMode(voltagePin, OUTPUT);
    digitalWrite(voltagePin, LOW);
#endif

#ifdef enablePin
    // Set state to active
    pinMode(enablePin, OUTPUT);
    digitalWrite(enablePin, HIGH);
#endif // enablePin

  // Start communication
  DEBUG_STREAM.begin(baud);
  MODEM_STREAM.begin(baud);
}

// Forward every message to the other serial
void loop() 
{
  while (DEBUG_STREAM.available())
  {
    MODEM_STREAM.write(DEBUG_STREAM.read());
  }

  while (MODEM_STREAM.available())
  {     
    DEBUG_STREAM.write(MODEM_STREAM.read());
  }
  
#ifndef ARDUINO_AVR_SODAQ_MBILI
  // check if the USB virtual serial wants a new baud rate
  if (DEBUG_STREAM.baud() != baud) {
    baud = DEBUG_STREAM.baud();
    MODEM_STREAM.begin(baud);
  }
#endif // !ARDUINO_AVR_SODAQ_MBILI
}

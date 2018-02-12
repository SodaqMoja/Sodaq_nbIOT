#include <Arduino.h>
#include <Wire.h>
// #include <SoftwareSerial.h> // Uno
#include <Sodaq_nbIOT.h>
#include <Sodaq_HTS221.h>

#if defined(ARDUINO_AVR_LEONARDO)
#define DEBUG_STREAM Serial 
#define MODEM_STREAM Serial1

#elif defined(ARDUINO_AVR_UNO)
SoftwareSerial softSerial(10, 11); // RX, TX
// You can connect an uartsbee or other board (e.g. 2nd Uno) to connect the softserial.
#define DEBUG_STREAM softSerial 
#define MODEM_STREAM Serial

#elif defined(ARDUINO_SODAQ_EXPLORER)
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial

#elif defined(ARDUINO_SAM_ZERO)
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1

#else
#error "Please select one of the listed boards."
#endif

Sodaq_nbIOT nbiot;
Sodaq_HTS221 humiditySensor;

void setup()
{
    while ((!DEBUG_STREAM) && (millis() < 10000)) {
        // Wait for serial monitor for 10 seconds
    }

    Wire.begin();

    DEBUG_STREAM.begin(9600);
    MODEM_STREAM.begin(nbiot.getDefaultBaudrate());

    DEBUG_STREAM.println("\r\nSODAQ Humidity and Temperature Example\r\n");

    nbiot.init(MODEM_STREAM, 7);
    nbiot.setDiag(DEBUG_STREAM);

    delay(2000);

    if (nbiot.connect("oceanconnect.t-mobile.nl", "172.16.14.22", "20416")) {
        DEBUG_STREAM.println("Connected succesfully!");
    }
    else {
        DEBUG_STREAM.println("Failed to connect!");
    }

    if (humiditySensor.init()) {
        DEBUG_STREAM.println("Temperature + humidity sensor initialized.");
        humiditySensor.enableSensor();
    }
    else {
        DEBUG_STREAM.println("Temperature + humidity initialization failed!");
    }
}


void loop()
{
    // Create the message
    String message = String(humiditySensor.readTemperature()) + "C" +
        ",  " + String(humiditySensor.readHumidity()) + "%";

    // Print the message we want to send
    DEBUG_STREAM.println(message);

    // Send the message
    nbiot.sendMessage(message);

    // Wait some time between messages
    delay(10000); // 1000 = 1 sec
}

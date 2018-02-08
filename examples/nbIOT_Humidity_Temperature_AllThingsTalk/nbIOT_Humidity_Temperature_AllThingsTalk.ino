#include <Arduino.h>
#include <Wire.h>
#include <Sodaq_wdt.h>
#include <Sodaq_nbIOT.h>
#include <Sodaq_HTS221.h>
#if defined(ARDUINO_AVR_UNO)
#include <SoftwareSerial.h> // Uno
#endif

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
#error "Please select a Sodaq ExpLoRer, Arduino Leonardo or add your board."
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

    DEBUG_STREAM.println("\r\nSODAQ Temperature and Humidity AllThingsTalk Example\r\n");

    nbiot.init(MODEM_STREAM, 7);
    nbiot.setDiag(DEBUG_STREAM);

    delay(2000);

    if (nbiot.connect("oceanconnect.t-mobile.nl", "172.16.14.22", "20416")) {
        DEBUG_STREAM.println("Connected succesfully!");
    }
    else {
        DEBUG_STREAM.println("Failed to connect!");
        return;
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
    byte message[8];
    uint16_t cursor = 0;
    int16_t temperature;
    int16_t humidity;

    temperature = humiditySensor.readTemperature() * 100;
    DEBUG_STREAM.println(temperature);
    message[cursor++] = temperature >> 8;
    message[cursor++] = temperature;

    humidity = humiditySensor.readHumidity() * 100;
    DEBUG_STREAM.println(humidity);
    message[cursor++] = humidity >> 8;
    message[cursor++] = humidity;

    // Print the message we want to send
    // DEBUG_STREAM.println(message);
    for (int i = 0; i < cursor; i++) {
        if (message[i] < 10) {
            DEBUG_STREAM.print("0");
        }
        DEBUG_STREAM.print(message[i], HEX);
        DEBUG_STREAM.print(":");
    }
    DEBUG_STREAM.println();

    // Send the message
    nbiot.sendMessage(message, cursor);

    // Wait some time between messages
    delay(10000); // 1000 = 1 sec
}

/*****
* ATT Settings
*
* create a new asset as Number
*
* device decoding:

{
"sense": [
{
"asset": "temperature",
"value" : {
"byte": 0,
"bytelength" : 2,
"type" : "integer",
"calculation" : "val / 100"
}
},
{
"asset": "humidity",
"value" : {
"byte": 2,
"bytelength" : 2,
"type" : "integer",
"calculation" : "val / 100"
}
}
]
}
*/
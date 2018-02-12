#include <Arduino.h>
#include <Wire.h>
#include <Sodaq_nbIOT.h>
#include <Sodaq_HTS221.h>
#include <Sodaq_LPS22HB.h>
#include <Sodaq_UBlox_GPS.h>
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
#error "Please select one of the listed boards."
#endif

Sodaq_nbIOT nbiot;
Sodaq_HTS221 humiditySensor;
Sodaq_LPS22HB barometricSensor;

uint32_t lat = 0;
uint32_t lon = 0;

void setup();
bool connectToNetwork();
void initHumidityTemperature();
void initPressureSensor();
void initGPS();
void loop();
void do_flash_led(int pin);

void setup()
{
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);

    while ((!DEBUG_STREAM) && (millis() < 10000)) {
        // Wait for serial monitor for 10 seconds
    }

    DEBUG_STREAM.begin(9600);
    MODEM_STREAM.begin(nbiot.getDefaultBaudrate());

    DEBUG_STREAM.println("\r\nSODAQ NB-IoT Shield AllThingsTalk Example\r\n");

    Wire.begin();

    nbiot.init(MODEM_STREAM, 7);
    nbiot.setDiag(DEBUG_STREAM);

    delay(2000);

    while (!connectToNetwork());

    initHumidityTemperature();
    initPressureSensor();
    initGPS();

    digitalWrite(13, LOW);
}

bool connectToNetwork() {
    if (nbiot.connect("oceanconnect.t-mobile.nl", "172.16.14.22", "20416")) {
        DEBUG_STREAM.println("Connected succesfully!");
        return true;
    }
    else {
        DEBUG_STREAM.println("Failed to connect!");
        delay(2000);
        return false;
    }
}

void initHumidityTemperature() {
    if (humiditySensor.init()) {
        DEBUG_STREAM.println("Temperature + humidity sensor initialized.");
        humiditySensor.enableSensor();
    }
    else {
        DEBUG_STREAM.println("Temperature + humidity initialization failed!");
    }
}

void initPressureSensor() {
    if (barometricSensor.init()) {
        DEBUG_STREAM.println("Barometric sensor initialization succeeded!");
        barometricSensor.enableSensor(Sodaq_LPS22HB::OdrOneShot);
    }
    else {
        DEBUG_STREAM.println("Barometric sensor initialization failed!");
    }
}

void initGPS() {
    sodaq_gps.init(6);
    // sodaq_gps.setDiag(DEBUG_STREAM);
}


void loop()
{
    do_flash_led(13);
    // Create the message
    byte message[14];
    uint16_t cursor = 0;
    int16_t temperature;
    int16_t humidity;
    int16_t pressure;

    temperature = humiditySensor.readTemperature() * 100;
    DEBUG_STREAM.println("Temperature x100 : " + (String)temperature);
    message[cursor++] = temperature >> 8;
    message[cursor++] = temperature;

    delay(100);

    humidity = humiditySensor.readHumidity() * 100;
    DEBUG_STREAM.println("Humidity x100 : " + (String)humidity);
    message[cursor++] = humidity >> 8;
    message[cursor++] = humidity;

    delay(100);

    pressure = barometricSensor.readPressureHPA();
    DEBUG_STREAM.println("Pressure:" + (String)pressure);
    message[cursor++] = pressure >> 8;
    message[cursor++] = pressure;

    uint32_t start = millis();
    uint32_t timeout = 1UL * 10 * 1000; // 10 sec timeout

    DEBUG_STREAM.println(String("waiting for fix ..., timeout=") + timeout + String("ms"));
    if (sodaq_gps.scan(true, timeout)) {

        lat = sodaq_gps.getLat() * 100000;
        lon = sodaq_gps.getLon() * 100000;
    }
    else {
        DEBUG_STREAM.println("No Fix");
    }

    message[cursor++] = lat >> 24;
    message[cursor++] = lat >> 16;
    message[cursor++] = lat >> 8;
    message[cursor++] = lat;


    message[cursor++] = lon >> 24;
    message[cursor++] = lon >> 16;
    message[cursor++] = lon >> 8;
    message[cursor++] = lon;

    // Print the message we want to send
    // DEBUG_STREAM.println(message);
    for (int i = 0; i < cursor; i++) {
        if (message[i] < 0x10) {
            DEBUG_STREAM.print("0");
        }
        DEBUG_STREAM.print(message[i], HEX);
        if (i < (cursor - 1)) {
            DEBUG_STREAM.print(":");
        }
    }
    DEBUG_STREAM.println();

    // Send the message
    nbiot.sendMessage(message, cursor);

    // Wait some time between messages
    delay(10000); // 1000 = 1 sec
}

void do_flash_led(int pin)
{
    for (size_t i = 0; i < 2; ++i) {
        delay(100);
        digitalWrite(pin, LOW);
        delay(100);
        digitalWrite(pin, HIGH);
        delay(100);
        digitalWrite(pin, LOW);
    }
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
 "asset": "my_temperature",
 "value": {
 "byte": 0,
 "bytelength": 2,
 "type": "integer",
 "calculation": "val / 100"
 }
 },
 {
 "asset": "my_humidity",
 "value": {
 "byte": 2,
 "bytelength": 2,
 "type": "integer",
 "calculation": "val / 100"
 }
 },
 {
 "asset": "my_pressure",
 "value": {
 "byte": 4,
 "bytelength": 2,
 "type": "integer"
 }
 },
 {
 "asset": "my_gps",
 "value": {
 "latitude": {
 "byte": 6,
 "bytelength": 4,
 "type": "integer",
 "calculation": "val / 100000"
 },
 "longitude": {
 "byte": 10,
 "bytelength": 4,
 "type": "integer",
 "calculation": "val / 100000"
 }
 }
 }
 ]
 }
 */

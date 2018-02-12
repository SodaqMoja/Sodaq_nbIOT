#include <Wire.h>
#include <Sodaq_nbIOT.h>
#include <Sodaq_LPS22HB.h>

#if defined(ARDUINO_AVR_LEONARDO)
#define DEBUG_STREAM Serial 
#define MODEM_STREAM Serial1

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
Sodaq_LPS22HB barometricSensor;

void setup()
{
    while ((!DEBUG_STREAM) && (millis() < 10000)) {
        // Wait for serial monitor for 10 seconds
    }

    Wire.begin();

    DEBUG_STREAM.begin(9600);
    MODEM_STREAM.begin(nbiot.getDefaultBaudrate());

    DEBUG_STREAM.println("\r\nSODAQ LPS22HB Arduino Example\r\n");

    nbiot.init(MODEM_STREAM, 7);
    nbiot.setDiag(DEBUG_STREAM);

    if (nbiot.connect("oceanconnect.t-mobile.nl", "172.16.14.22", "20416")) {
        DEBUG_STREAM.println("Connected succesfully!");
    }
    else {
        DEBUG_STREAM.println("Failed to connect!");
    }

    if (barometricSensor.init()) {
        DEBUG_STREAM.println("Barometric sensor initialization succeeded!");
        barometricSensor.enableSensor(Sodaq_LPS22HB::OdrOneShot);
    }
    else {
        DEBUG_STREAM.println("Barometric sensor initialization failed!");
    }

    DEBUG_STREAM.println("Done with setup!");
}

void loop()
{
    // Create the message
    String message = String(barometricSensor.readPressureHPA()) + " mbar";

    // Print the message we want to send
    DEBUG_STREAM.println(message);

    // Send the message
    nbiot.sendMessage(message);

    // Wait some time between messages
    delay(10000); // 1000 = 1 sec
}

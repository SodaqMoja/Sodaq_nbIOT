/*
Copyright (c) 2015-2016 Sodaq.  All rights reserved.

This file is part of Sodaq_nbIOT.

Sodaq_nbIOT is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or(at your option) any later version.

Sodaq_nbIOT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with Sodaq_nbIOT.  If not, see
<http://www.gnu.org/licenses/>.
*/

#include "Sodaq_nbIOT.h"
#include "Sodaq_wdt.h"

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

#else
#error "Please use one of the listed boards or add your board."
#endif

#define DEBUG_STREAM SerialUSB
#define DEBUG_STREAM_BAUD 115200

#define STARTUP_DELAY 5000

const char* apn = "nb.inetd.gdsp";
const char* cdp = "172.16.14.22";
const uint8_t band = 20;
const char* forceOperator = "20404"; // optional - depends on SIM / network

Sodaq_nbIOT nbiot;

void sendMessageThroughUDP()
{
    DEBUG_STREAM.println();
    DEBUG_STREAM.println("Sending message through UDP");

    int localPort = 16666;
    int socketID = nbiot.createSocket(localPort);

    if (socketID >= 7 || socketID < 0) {
        DEBUG_STREAM.println("Failed to create socket");
        return;
    }

    DEBUG_STREAM.println("Created socket!");

    char* buffer = "FFAAFF";
    uint8_t size = strlen(buffer) / 2;
    // "195.34.89.241" : 7 is the ublox echo service
    int lengthSent = nbiot.socketSend(socketID, "195.34.89.241", 7, buffer, size);
    DEBUG_STREAM.print("Length buffer vs sent:");
    DEBUG_STREAM.print(size);
    DEBUG_STREAM.println(lengthSent);

   // wait for data
    if (nbiot.waitForUDPResponse()) {
        DEBUG_STREAM.println("GOT RESPONSE");

        while (nbiot.hasPendingUDPBytes()) {

            char data[200];
            // read two bytes at a time
            SaraN2UDPPacketMetadata p;
            int size = nbiot.socketReceiveHex(data, 2, &p);

            if (size) {
                DEBUG_STREAM.println(data);
                // p is a pointer to memory that is owned by nbiot class
                DEBUG_STREAM.println(p.socketID);
                DEBUG_STREAM.println(p.ip);
                DEBUG_STREAM.println(p.port);
                DEBUG_STREAM.println(p.length);
                DEBUG_STREAM.println(p.remainingLength);
            }
            else {
                DEBUG_STREAM.println("Receive failed!");
            }
        }
    }
    else {
        DEBUG_STREAM.println("TIMEOUT!!");
    }

    nbiot.closeSocket(socketID);
    DEBUG_STREAM.println();
}

void setup()
{
    sodaq_wdt_safe_delay(STARTUP_DELAY);

    DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
    MODEM_STREAM.begin(nbiot.getDefaultBaudrate());

    DEBUG_STREAM.println("Initializing and connecting... ");

    nbiot.init(MODEM_STREAM, powerPin);
    nbiot.setDiag(DEBUG_STREAM);

    pinMode(SARA_TX_ENABLE, OUTPUT);
    pinMode(SARA_RESET, OUTPUT);

    digitalWrite(SARA_TX_ENABLE, HIGH);
    digitalWrite(SARA_RESET, HIGH);
    
    if (!nbiot.connect(apn, cdp, forceOperator, band)) {
        DEBUG_STREAM.println("FAILED TO CONNECT TO MODEM");
    }

    sendMessageThroughUDP();
}

void loop()
{
    sodaq_wdt_safe_delay(5000);
}

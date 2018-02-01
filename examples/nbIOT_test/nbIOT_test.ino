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
#include <Sodaq_wdt.h>

#ifdef ARDUINO_SODAQ_EXPLORER
/* SODAQ Explorer + SODAQ NB-IoT Shield */
#define DEBUG_STREAM SerialUSB
#define MODEM_ON_OFF_PIN 7
#define MODEM_STREAM Serial

#elif defined(ARDUINO_SAM_ZERO)
/* Arduino Zero / M0 + SODAQ NB-IoT Shield */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define MODEM_ON_OFF_PIN 7 

#elif defined(ARDUINO_AVR_LEONARDO)
/* Arduino Leonardo + SODAQ NB-IoT Shield */
#define DEBUG_STREAM Serial 
#define MODEM_STREAM Serial1
#define MODEM_ON_OFF_PIN 7

#elif defined(ARDUINO_SODAQ_AUTONOMO)
/* SODAQ AUTONOMO + SODAQ NB-IoT Bee */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define MODEM_ON_OFF_PIN BEE_VCC
#define MODEM_DTR BEEDTR

#elif defined(ARDUINO_AVR_SODAQ_MBILI)
/* SODAQ MBILI + SODAQ NB-IoT Bee */
#define DEBUG_STREAM Serial
#define MODEM_STREAM Serial1
#define MODEM_DTR BEEDTR

#elif defined(ARDUINO_SODAQ_SARA)
/* SODAQ SARA */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define MODEM_ON_OFF_PIN SARA_ENABLE
#define MODEM_DTR SARA_TX_ENABLE

#else
#error "You need to declare the modem on/off pin and stream for your particular board!"
#endif


#define DEBUG_STREAM_BAUD 115200

#define STARTUP_DELAY 5000

const char* apn = "oceanconnect.t-mobile.nl";
const char* cdp = "172.16.14.22";
const char* forceOperator = "20416"; // optional - depends on SIM / network

Sodaq_nbIOT nbiot;

void showMessageCountFromModem();

void setup()
{
    sodaq_wdt_safe_delay(STARTUP_DELAY);

    DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
    MODEM_STREAM.begin(nbiot.getDefaultBaudrate());

    DEBUG_STREAM.print("Initializing and connecting... ");

#ifdef MODEM_DTR
    // Set state to active
    pinMode(MODEM_DTR, OUTPUT);
    digitalWrite(MODEM_DTR, HIGH);
#endif // MODEM_DTR

    nbiot.init(MODEM_STREAM, MODEM_ON_OFF_PIN);
    nbiot.setDiag(DEBUG_STREAM);

    connectModem();
}

void loop()
{
    if (nbiot.isConnected()) {
        const char* message = "Hello World!";
        DEBUG_STREAM.print("Sending message: \"");
        DEBUG_STREAM.print(message);
        DEBUG_STREAM.print("\"... ");

        if (!nbiot.sendMessage(message)) {
            DEBUG_STREAM.println("Could not queue message!");
        }
        else {
            DEBUG_STREAM.println("Message queued for transmission!");
        }

        showMessageCountFromModem();
    }
    else {
        connectModem();
    }

    sodaq_wdt_safe_delay(5000);
}

void connectModem() {
    if (nbiot.connect(apn, cdp, forceOperator)) {
        DEBUG_STREAM.println("Connected succesfully!");
    }
    else {
        DEBUG_STREAM.println("Failed to connect!");
        return;
    }
}

void showMessageCountFromModem()
{
    DEBUG_STREAM.print("Pending Messages: ");
    DEBUG_STREAM.print(nbiot.getSentMessagesCount(Sodaq_nbIOT::Pending));
    DEBUG_STREAM.print("  |  Failed Messages: ");
    DEBUG_STREAM.println(nbiot.getSentMessagesCount(Sodaq_nbIOT::Error));
}

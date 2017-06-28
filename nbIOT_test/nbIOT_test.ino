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

#define MODEM_ON_OFF_PIN 7
#define MODEM_STREAM Serial

#define DEBUG_STREAM SerialUSB
#define DEBUG_STREAM_BAUD 115200

const char* apn = "oceanconnect.t-mobile.nl";
const char* cdp = "172.16.14.20";
const char* forceOperator = "20416"; // optional - depends on SIM / network

Sodaq_nbIOT nbiot;

void showMessageCountFromModem();

void setup()
{
    DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
    MODEM_STREAM.begin(nbiot.getDefaultBaudrate());

    nbiot.init(MODEM_STREAM, MODEM_ON_OFF_PIN);
    nbiot.setDiag(DEBUG_STREAM);

    if (nbiot.connect(apn, cdp, forceOperator)) {
        DEBUG_STREAM.println("Connected succesfully!");
    }
    else {
        DEBUG_STREAM.println("Failed to connect!");
        return;
    }

    showMessageCountFromModem();

    if (!nbiot.sendMessage("Hello World")) {
        DEBUG_STREAM.println("Could not queue message!");
    }
}

void loop()
{
    if (nbiot.isConnected()) {
        showMessageCountFromModem();
    }

    sodaq_wdt_safe_delay(5000);
}

void showMessageCountFromModem()
{
    DEBUG_STREAM.print("Pending Messages: ");
    DEBUG_STREAM.print(nbiot.getSentMessagesCount(Sodaq_nbIOT::Pending));
    DEBUG_STREAM.print("  |  Error Messages: ");
    DEBUG_STREAM.println(nbiot.getSentMessagesCount(Sodaq_nbIOT::Error));
}

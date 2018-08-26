# Sodaq_nbIOT v2

Arduino library for using with the uBlox nbIOT modules.

The firmware version of the module should be >= 06.57,A03.02.

Please check http://support.sodaq.com/sodaq-one/firmware-upgrade/ for the latest firmware.

## Usage

Quick example:

```c
#include "Sodaq_nbIOT.h"
#include <Sodaq_wdt.h>

#ifdef ARDUINO_SODAQ_EXPLORER
#define MODEM_ON_OFF_PIN 7
#define MODEM_STREAM Serial
#else
#error "You need to declare the modem on/off pin and stream for your particular board!"
#endif

#define DEBUG_STREAM SerialUSB
#define DEBUG_STREAM_BAUD 115200

const char* apn = "oceanconnect.t-mobile.nl";
const char* cdp = "172.16.14.22";
const char* forceOperator = "20416"; // optional - depends on SIM / network

Sodaq_nbIOT nbiot;

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

    const char* message = "Hello World!";

    if (!nbiot.sendMessage(message)) {
        DEBUG_STREAM.println("Could not queue message!");
    }
    else {
        DEBUG_STREAM.println("Message queued for transmission!");
    }
}

void loop()
{
}

```

Method|Description
------|------
**getDefaultBaudRate ()**|Returns the correct baudrate for the serial port that connects to the device.
**setDiag (Stream& stream)**|Sets the optional "Diagnostics and Debug" stream.
**init(Stream& stream, int8_t onoffPin)**|    // Initializes the modem instance. Sets the modem stream and the on-off power pins.
**overrideNconfigParam(const char\* param, bool value)**|Override a default config parameter, has to be called before connect(). Returns false if the parameter name was not found. Possible values for param are: AUTOCONNECT, CR_0354_0338_SCRAMBLING, CR_0859_SI_AVOID, COMBINE_ATTACH, CELL_RESELECTION and ENABLE_BIP.
**isAlive()**|Returns true if the modem replies to "AT" commands without timing out.
**connect(const char\* apn, const char\* cdp, const char\* forceOperator = 0, uint8_t band = 8)**|Turns on and initializes the modem, then connects to the network and activates the data connection. Returns true when successful.
**disconnect()**|Disconnects the modem from the network. Returns true when successful.
**isConnected()**|Returns true if the modem is connected to the network and has an activated data connection.
**sendMessage(const uint8_t\* buffer, size_t size)**|Sends the given buffer, up to "size" bytes long. Returns true when the message is successfully queued for transmission on the modem.
**sendMessage(const char\* str)**|Sends the given null-terminated c-string. Returns true when the message is successfully queued for transmission on the modem.
**sendMessage(String str)**|Sends the given String. Returns true when the message is successfully queued for transmission on the modem.
**getSentMessagesCount(SentMessageStatus filter)**|Returns the number of messages that are either pending (filter == Pending) or failed to be transmitted (filter == Error) on the modem.
**createSocket(uint16_t localPort = 0)**|Create a UDP socket for the specified local port, returns the socket handle.
**closeSocket(uint8_t socket)**|Close a UDP socket by handle, returns true if successful.
**socketSend(uint8_t socket, const char\* remoteIP, const uint16_t remotePort,  const uint8_t\* buffer, size_t size)**|Send a UDP payload buffer to a specified remote IP and port, through a specific socket.
**socketSend(uint8_t socket, const char\* remoteIP, const uint16_t remotePort, const char\* str)**|Send a UDP string to a specified remote IP and port, through a specific socket.
**socketReceiveHex(char\* buffer, size_t length, SaraN2UDPPacketMetadata\* p = NULL)**|Receive pending socket data as hex data in a passed buffer. Optionally pass a helper object to receive metadata about the origin of the socket data.
**socketReceiveBytes(uint8_t\* buffer, size_t length, SaraN2UDPPacketMetadata\* p = NULL)**|Receive pending socket data as binary data in a passed buffer. Optionally pass a helper object to receive metadata about the origin of the socket data.
**getPendingUDPBytes()**| Return the number of pending bytes, gets updated by calling socketReceiveXXX.
**hasPendingUDPBytes()**| Helper function returning if getPendingUDPBytes() > 0.
**ping(char\* ip)**| Ping a specific IP address.
**waitForUDPResponse(uint32_t timeoutMS = DEFAULT_UDP_TIMOUT_MS)**|Calls isAlive() until the passed timeout, or until a UDP packet has been received on any socket.

## Contributing

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request

## License

Copyright (c) 2018 Sodaq.  All rights reserved.

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

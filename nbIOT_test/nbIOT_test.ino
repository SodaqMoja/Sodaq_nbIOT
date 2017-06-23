#include "Sodaq_nbIOT.h"
#include <Sodaq_wdt.h>

#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial

Sodaq_nbIOT nbiot;

// the setup function runs once when you press reset or power the board
void setup() {
    DEBUG_STREAM.begin(115200);
    MODEM_STREAM.begin(nbiot.getDefaultBaudrate());
    
    nbiot.init(MODEM_STREAM, 7);
    nbiot.setDiag(DEBUG_STREAM);

    if (nbiot.connect("oceanconnect.t-mobile.nl", "172.16.14.20", "20416")) {
        DEBUG_STREAM.println("Connected succesfully!");
    }
    else {
        DEBUG_STREAM.println("Failed to connect!");
        return;
    }

    DEBUG_STREAM.print("Pending Messages: ");
    DEBUG_STREAM.println(nbiot.getSentMessagesCount(Sodaq_nbIOT::Pending));
    DEBUG_STREAM.print("Error Messages: ");
    DEBUG_STREAM.println(nbiot.getSentMessagesCount(Sodaq_nbIOT::Error));

    if (!nbiot.sendMessage("Hello World")) {
        DEBUG_STREAM.println("Could not queue message!");
    }
}

void loop() 
{
    DEBUG_STREAM.print("Pending Messages: ");
    DEBUG_STREAM.println(nbiot.getSentMessagesCount(Sodaq_nbIOT::Pending));
    DEBUG_STREAM.print("Error Messages: ");
    DEBUG_STREAM.println(nbiot.getSentMessagesCount(Sodaq_nbIOT::Error));

    sodaq_wdt_safe_delay(1000);
}

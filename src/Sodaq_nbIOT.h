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

#ifndef _Sodaq_nbIOT_h
#define _Sodaq_nbIOT_h

#include "Arduino.h"
#include "Sodaq_AT_Device.h"

class Sodaq_nbIOT: public Sodaq_AT_Device
{
  public:
    Sodaq_nbIOT();

    enum SentMessageStatus {
        Pending,
        Error
    };

    typedef ResponseTypes(*CallbackMethodPtr)(ResponseTypes& response, const char* buffer, size_t size,
            void* parameter, void* parameter2);

    bool setRadioActive(bool on);
    bool setIndicationsActive(bool on);
    bool setApn(const char* apn);
    bool setCdp(const char* cdp);

    // Returns true if the modem replies to "AT" commands without timing out.
    bool isAlive();

    // Returns the default baud rate of the modem.
    // To be used when initializing the modem stream for the first time.
    uint32_t getDefaultBaudrate() { return 9600; };

    // Initializes the modem instance. Sets the modem stream and the on-off power pins.
    void init(Stream& stream, int8_t onoffPin);

    // Turns on and initializes the modem, then connects to the network and activates the data connection.
    bool connect(const char* apn, const char* cdp, const char* forceOperator = 0);

    // Disconnects the modem from the network.
    bool disconnect();

    // Returns true if the modem is connected to the network and has an activated data connection.
    bool isConnected();

    // Gets the Received Signal Strength Indication in dBm and Bit Error Rate.
    // Returns true if successful.
    bool getRSSIAndBER(int8_t* rssi, uint8_t* ber);
    int8_t convertCSQ2RSSI(uint8_t csq) const;
    uint8_t convertRSSI2CSQ(int8_t rssi) const;

    void setMinRSSI(int rssi) { _minRSSI = rssi; }
    void setMinCSQ(int csq) { _minRSSI = convertCSQ2RSSI(csq); }
    int8_t getMinRSSI() const { return _minRSSI; }
    uint8_t getCSQtime() const { return _CSQtime; }
    int8_t getLastRSSI() const { return _lastRSSI; }

    //int createSocket(uint16_t localPort = 0);
    //bool connectSocket(uint8_t socket, const char* host, uint16_t port);
    //bool socketSend(uint8_t socket, const uint8_t* buffer, size_t size);
    //size_t socketReceive(uint8_t socket, uint8_t* buffer, size_t size);
    //size_t socketBytesPending(uint8_t socket);
    //bool closeSocket(uint8_t socket);

    bool sendMessage(const uint8_t* buffer, size_t size);
    bool sendMessage(const char* str);
    bool sendMessage(String str);
    int getSentMessagesCount(SentMessageStatus filter);
  protected:
    // override
    ResponseTypes readResponse(char* buffer, size_t size, size_t* outSize, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
    };

    ResponseTypes readResponse(char* buffer, size_t size,
                               CallbackMethodPtr parserMethod, void* callbackParameter, void* callbackParameter2 = NULL,
                               size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS);

    ResponseTypes readResponse(size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
    };

    ResponseTypes readResponse(CallbackMethodPtr parserMethod, void* callbackParameter,
                               void* callbackParameter2 = NULL, size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize,
                            parserMethod, callbackParameter, callbackParameter2,
                            outSize, timeout);
    };

    template<typename T1, typename T2>
    ResponseTypes readResponse(ResponseTypes(*parserMethod)(ResponseTypes& response, const char* parseBuffer, size_t size, T1* parameter, T2* parameter2),
                               T1* callbackParameter, T2* callbackParameter2,
                               size_t* outSize = NULL, uint32_t timeout = SODAQ_AT_DEVICE_DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, (CallbackMethodPtr)parserMethod,
                            (void*)callbackParameter, (void*)callbackParameter2, outSize, timeout);
    };

    void purgeAllResponsesRead();
  private:
    //uint16_t _socketPendingBytes[SOCKET_COUNT]; // TODO add getter
    //bool _socketClosedBit[SOCKET_COUNT];

    // This is the value of the most recent CSQ
    // Notice that CSQ is somewhat standard. SIM800/SIM900 and Ublox
    // compute to comparable numbers. With minor deviations.
    // For example SIM800
    //   1              -111 dBm
    //   2...30         -110... -54 dBm
    // For example UBlox
    //   1              -111 dBm
    //   2..30          -109 to -53 dBm
    int8_t _lastRSSI;   // 0 not known or not detectable

    // This is the number of second it took when CSQ was record last
    uint8_t _CSQtime;

    // This is the minimum required RSSI to continue making the connection
    // Use convertCSQ2RSSI if you have a CSQ value
    int _minRSSI;

    static bool startsWith(const char* pre, const char* str);
    static size_t ipToString(IP_t ip, char* buffer, size_t size);
    static bool isValidIPv4(const char* str);

    bool waitForSignalQuality(uint32_t timeout = 60L * 1000);
    bool attachGprs(uint32_t timeout = 30 * 1000);
    bool setNconfigParam(const char* param, const char* value);
    bool checkAndApplyNconfig();
    void reboot();

    static ResponseTypes _csqParser(ResponseTypes& response, const char* buffer, size_t size, int* rssi, int* ber);
    //static ResponseTypes _createSocketParser(ResponseTypes& response, const char* buffer, size_t size,
    //        uint8_t* socket, uint8_t* dummy);
    static ResponseTypes _nqmgsParser(ResponseTypes& response, const char* buffer, size_t size, uint16_t* pendingCount, uint16_t* errorCount);
    static ResponseTypes _cgattParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* result, uint8_t* dummy);
    static ResponseTypes _nconfigParser(ResponseTypes& response, const char* buffer, size_t size, bool* nconfigEqualsArray, uint8_t* dummy);
};

#endif


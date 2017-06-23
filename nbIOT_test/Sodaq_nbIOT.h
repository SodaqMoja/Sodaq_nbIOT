#ifndef _Sodaq_nbIOT_h
#define _Sodaq_nbIOT_h

#include "Arduino.h"
#include "Sodaq_GSM_Modem.h"

#define SOCKET_COUNT 7 // TODO

class Sodaq_nbIOT: public Sodaq_GSM_Modem
{
public:
    enum Filter {
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

    bool attachGprs(uint32_t timeout = 30 * 1000);

    // Gets the Received Signal Strength Indication in dBm and Bit Error Rate.
    // Returns true if successful.
    bool getRSSIAndBER(int8_t* rssi, uint8_t* ber);
    int8_t convertCSQ2RSSI(uint8_t csq) const;
    uint8_t convertRSSI2CSQ(int8_t rssi) const;

    int createSocket(uint16_t localPort = 0);
    bool connectSocket(uint8_t socket, const char* host, uint16_t port);
    //bool socketSend(uint8_t socket, const uint8_t* buffer, size_t size);
    //size_t socketReceive(uint8_t socket, uint8_t* buffer, size_t size);
    //size_t socketBytesPending(uint8_t socket);
    //bool closeSocket(uint8_t socket);

    bool sendMessage(const uint8_t* buffer, size_t size);
    bool sendMessage(const char* str);
    bool sendMessage(String str);
    int getSentMessagesCount(Filter filter);

protected:
    // override
    ResponseTypes readResponse(char* buffer, size_t size, size_t* outSize, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
    };

    ResponseTypes readResponse(char* buffer, size_t size,
        CallbackMethodPtr parserMethod, void* callbackParameter, void* callbackParameter2 = NULL,
        size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS);

    ResponseTypes readResponse(size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, NULL, NULL, NULL, outSize, timeout);
    };

    ResponseTypes readResponse(CallbackMethodPtr parserMethod, void* callbackParameter,
        void* callbackParameter2 = NULL, size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize,
            parserMethod, callbackParameter, callbackParameter2,
            outSize, timeout);
    };

    template<typename T1, typename T2>
    ResponseTypes readResponse(ResponseTypes(*parserMethod)(ResponseTypes& response, const char* parseBuffer, size_t size, T1* parameter, T2* parameter2),
        T1* callbackParameter, T2* callbackParameter2,
        size_t* outSize = NULL, uint32_t timeout = DEFAULT_READ_MS)
    {
        return readResponse(_inputBuffer, _inputBufferSize, (CallbackMethodPtr)parserMethod,
            (void*)callbackParameter, (void*)callbackParameter2, outSize, timeout);
    };

private:
    //uint16_t _socketPendingBytes[SOCKET_COUNT]; // TODO add getter
    //bool _socketClosedBit[SOCKET_COUNT];
    //uint32_t _timeToSocketConnect;
    //uint32_t _timeToSocketClose;

    static bool startsWith(const char* pre, const char* str);
    static size_t ipToString(IP_t ip, char* buffer, size_t size);
    static bool isValidIPv4(const char* str);

    bool waitForSignalQuality(uint32_t timeout = 60L * 1000);

    static ResponseTypes _csqParser(ResponseTypes& response, const char* buffer, size_t size, int* rssi, int* ber);
    static ResponseTypes _createSocketParser(ResponseTypes& response, const char* buffer, size_t size,
        uint8_t* socket, uint8_t* dummy);
    static ResponseTypes _nqmgsParser(ResponseTypes& response, const char* buffer, size_t size, uint16_t* pendingCount, uint16_t* errorCount);
};

#endif


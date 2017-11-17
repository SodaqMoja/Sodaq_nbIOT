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

//#define DEBUG

#define STR_AT "AT"
#define STR_RESPONSE_OK "OK"
#define STR_RESPONSE_ERROR "ERROR"
#define STR_RESPONSE_CME_ERROR "+CME ERROR:"
#define STR_RESPONSE_CMS_ERROR "+CMS ERROR:"

#define DEBUG_STR_ERROR "[ERROR]: "

#define NIBBLE_TO_HEX_CHAR(i) ((i <= 9) ? ('0' + i) : ('A' - 10 + i))
#define HIGH_NIBBLE(i) ((i >> 4) & 0x0F)
#define LOW_NIBBLE(i) (i & 0x0F)

#define HEX_CHAR_TO_NIBBLE(c) ((c >= 'A') ? (c - 'A' + 0x0A) : (c - '0'))
#define HEX_PAIR_TO_BYTE(h, l) ((HEX_CHAR_TO_NIBBLE(h) << 4) + HEX_CHAR_TO_NIBBLE(l))

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#ifdef DEBUG
#define debugPrintLn(...) { if (!this->_disableDiag && this->_diagStream) this->_diagStream->println(__VA_ARGS__); }
#define debugPrint(...) { if (!this->_disableDiag && this->_diagStream) this->_diagStream->print(__VA_ARGS__); }
#warning "Debug mode is ON"
#else
#define debugPrintLn(...)
#define debugPrint(...)
#endif

#define DEFAULT_CID "1"

#define NO_IP_ADDRESS ((IP_t)0)

#define IP_FORMAT "%d.%d.%d.%d"

#define IP_TO_TUPLE(x) (uint8_t)(((x) >> 24) & 0xFF), \
    (uint8_t)(((x) >> 16) & 0xFF), \
    (uint8_t)(((x) >> 8) & 0xFF), \
    (uint8_t)(((x) >> 0) & 0xFF)

#define TUPLE_TO_IP(o1, o2, o3, o4) ((((IP_t)o1) << 24) | (((IP_t)o2) << 16) | \
                                     (((IP_t)o3) << 8) | (((IP_t)o4) << 0))

#define SOCKET_FAIL -1

#define SOCKET_COUNT 7

#define NOW (uint32_t)millis()

typedef struct NameValuePair {
    const char* Name;
    const char* Value;
} NameValuePair;

const uint8_t nConfigCount = 6;
static NameValuePair nConfig[nConfigCount] = {
    { "AUTOCONNECT", "FALSE" },
    { "CR_0354_0338_SCRAMBLING", "FALSE" },
    { "CR_0859_SI_AVOID", "FALSE" },
    { "COMBINE_ATTACH" , "FALSE" },
    { "CELL_RESELECTION" , "FALSE" },
    { "ENABLE_BIP" , "FALSE" },
};

class Sodaq_nbIotOnOff : public Sodaq_OnOffBee
{
  public:
    Sodaq_nbIotOnOff();
    void init(int onoffPin);
    void on();
    void off();
    bool isOn();
  private:
    int8_t _onoffPin;
    bool _onoff_status;
};

static Sodaq_nbIotOnOff sodaq_nbIotOnOff;

static inline bool is_timedout(uint32_t from, uint32_t nr_ms) __attribute__((always_inline));
static inline bool is_timedout(uint32_t from, uint32_t nr_ms)
{
    return (millis() - from) > nr_ms;
}

Sodaq_nbIOT::Sodaq_nbIOT() :
    _lastRSSI(0),
    _CSQtime(0),
    _minRSSI(-113) // dBm
{

}

// Returns true if the modem replies to "AT" commands without timing out.
bool Sodaq_nbIOT::isAlive()
{
    _disableDiag = true;
    println(STR_AT);

    return (readResponse(NULL, 450) == ResponseOK);
}

// Initializes the modem instance. Sets the modem stream and the on-off power pins.
void Sodaq_nbIOT::init(Stream& stream, int8_t onoffPin)
{
    debugPrintLn("[init] started.");

    initBuffer(); // safe to call multiple times

    setModemStream(stream);

    sodaq_nbIotOnOff.init(onoffPin);
    _onoff = &sodaq_nbIotOnOff;
}

bool Sodaq_nbIOT::setRadioActive(bool on)
{
    print("AT+CFUN=");
    println(on ? "1" : "0");

    return (readResponse() == ResponseOK);
}

bool Sodaq_nbIOT::setIndicationsActive(bool on)
{
    print("AT+NSMI=");
    println(on ? "1" : "0");

    if (readResponse() != ResponseOK) {
        return false;
    }

    print("AT+NNMI=");
    println(on ? "1" : "0");

    return (readResponse() == ResponseOK);
}

/*!
    Read the next response from the modem

    Notice that we're collecting URC's here. And in the process we could
    be updating:
      _socketPendingBytes[] if +UUSORD: is seen
      _socketClosedBit[] if +UUSOCL: is seen
*/
ResponseTypes Sodaq_nbIOT::readResponse(char* buffer, size_t size,
                                        CallbackMethodPtr parserMethod, void* callbackParameter, void* callbackParameter2,
                                        size_t* outSize, uint32_t timeout)
{
    ResponseTypes response = ResponseNotFound;
    uint32_t from = NOW;

    do {
        // 250ms,  how many bytes at which baudrate?
        int count = readLn(buffer, size, 250);
        sodaq_wdt_reset();

        if (count > 0) {
            if (outSize) {
                *outSize = count;
            }

            if (_disableDiag && strncmp(buffer, "OK", 2) != 0) {
                _disableDiag = false;
            }

            debugPrint("[rdResp]: ");
            debugPrintLn(buffer);

            // TODO handle socket URC

            //int param1, param2;
            //if (sscanf(buffer, "+UUSORD: %d,%d", &param1, &param2) == 2) {
            //    uint16_t socket_nr = param1;
            //    uint16_t nr_bytes = param2;
            //    debugPrint("Unsolicited: Socket ");
            //    debugPrint(socket_nr);
            //    debugPrint(": ");
            //    debugPrint(param2);
            //    debugPrintLn(" bytes pending");
            //    if (socket_nr < ARRAY_SIZE(_socketPendingBytes)) {
            //        _socketPendingBytes[socket_nr] = nr_bytes;
            //    }
            //    continue;
            //}
            //else if (sscanf(buffer, "+UUSOCL: %d", &param1) == 1) {
            //    uint16_t socket_nr = param1;
            //    if (socket_nr < ARRAY_SIZE(_socketPendingBytes)) {
            //        debugPrint("Unsolicited: Socket ");
            //        debugPrint(socket_nr);
            //        debugPrint(": ");
            //        debugPrintLn("closed by remote");

            //        _socketClosedBit[socket_nr] = true;
            //        if (socket_nr == _openTCPsocket) {
            //            _openTCPsocket = -1;
            //            // Report this other software layers
            //            if (_tcpClosedHandler) {
            //                _tcpClosedHandler();
            //            }
            //        }
            //    }
            //    continue;
            //}

            if (startsWith(STR_AT, buffer)) {
                continue; // skip echoed back command
            }

            _disableDiag = false;

            if (startsWith(STR_RESPONSE_OK, buffer)) {
                return ResponseOK;
            }

            if (startsWith(STR_RESPONSE_ERROR, buffer) ||
                    startsWith(STR_RESPONSE_CME_ERROR, buffer) ||
                    startsWith(STR_RESPONSE_CMS_ERROR, buffer)) {
                return ResponseError;
            }

            if (parserMethod) {
                ResponseTypes parserResponse = parserMethod(response, buffer, count, callbackParameter, callbackParameter2);

                if ((parserResponse != ResponseEmpty) && (parserResponse != ResponsePendingExtra)) {
                    return parserResponse;
                }
                else {
                    // ?
                    // ResponseEmpty indicates that the parser was satisfied
                    // Continue until "OK", "ERROR", or whatever else.
                }

                // Prevent calling the parser again.
                // This could happen if the input line is too long. It will be split
                // and the next readLn will return the next part.
                // The case of "ResponsePendingExtra" is an exception to this, thus waiting for more replies to be parsed.
                if (parserResponse != ResponsePendingExtra) {
                    parserMethod = 0;
                }
            }

            // at this point, the parserMethod has ran and there is no override response from it,
            // so if there is some other response recorded, return that
            // (otherwise continue iterations until timeout)
            if (response != ResponseNotFound) {
                debugPrintLn("** response != ResponseNotFound");
                return response;
            }
        }

        delay(10);      // TODO Why do we need this delay?
    } while (!is_timedout(from, timeout));

    if (outSize) {
        *outSize = 0;
    }

    debugPrintLn("[rdResp]: timed out");
    return ResponseTimeout;
}

bool Sodaq_nbIOT::setApn(const char* apn)
{
    print("AT+CGDCONT=" DEFAULT_CID ",\"IP\",\"");
    print(apn);
    println("\"");

    return (readResponse() == ResponseOK);
}

bool Sodaq_nbIOT::setCdp(const char* cdp)
{
    print("AT+NCDP=\"");
    print(cdp);
    println("\"");

    return (readResponse() == ResponseOK);
}

void Sodaq_nbIOT::purgeAllResponsesRead()
{
    uint32_t start = millis();

    // make sure all the responses within the timeout have been read
    while ((readResponse(0, 1000) != ResponseTimeout) && !is_timedout(start, 2000)) {}
}

// Turns on and initializes the modem, then connects to the network and activates the data connection.
bool Sodaq_nbIOT::connect(const char* apn, const char* cdp, const char* forceOperator)
{
    if (!on()) {
        return false;
    }

    purgeAllResponsesRead();

    if (!setRadioActive(false)) {
        return false;
    }

    if (!checkAndApplyNconfig()) {
        return false;
    }

    reboot();

    if (!on()) {
        return false;
    }

    purgeAllResponsesRead();

    if (!setApn(apn) || !setCdp(cdp)) {
        return false;
    }

    // TODO turn on
    if (!setIndicationsActive(false)) {
        return false;
    }

    if (!setRadioActive(true)) {
        return false;
    }
    
    if (forceOperator && forceOperator[0] != '\0') {
        print("AT+COPS=1,2,\"");
        print(forceOperator);
        println("\"");

        if (readResponse() != ResponseOK) {
            return false;
        }
    }

    if (!waitForSignalQuality()) {
        return false;
    }

    if (!attachGprs()) {
        return false;
    }

    #ifdef DEBUG
    println("AT+CPSMS?");
    readResponse();
    readResponse();
    #endif

    // If we got this far we succeeded
    return true;
}

void Sodaq_nbIOT::reboot()
{
    println("AT+NRB");

    // wait up to 2000ms for the modem to come up
    uint32_t start = millis();
    while ((readResponse() != ResponseOK) && !is_timedout(start, 2000)) { }
}

bool Sodaq_nbIOT::checkAndApplyNconfig()
{
    bool applyParam[nConfigCount];

    println("AT+NCONFIG?");

    if (readResponse<bool, uint8_t>(_nconfigParser, applyParam, NULL) == ResponseOK) {
        for (uint8_t i = 0; i < nConfigCount; i++)
        {
            debugPrint(nConfig[i].Name);
            if (!applyParam[i]) {
                debugPrintLn("... CHANGE");
                setNconfigParam(nConfig[i].Name, nConfig[i].Value);
            }
            else {
                debugPrintLn("... OK");
            }
        }

        return true;
    }

    return false;
}

bool Sodaq_nbIOT::setNconfigParam(const char* param, const char* value)
{
    print("AT+NCONFIG=\"");
    print(param);
    print("\",\"");
    print(value);
    println("\"");

    return readResponse() == ResponseOK;
}

ResponseTypes Sodaq_nbIOT::_nconfigParser(ResponseTypes& response, const char* buffer, size_t size, bool* nconfigEqualsArray, uint8_t* dummy)
{
    if (!nconfigEqualsArray) {
        return ResponseError;
    }

    char name[32];
    char value[32];
    if (sscanf(buffer, "+NCONFIG: \"%[^\"]\",\"%[^\"]\"", name, value) == 2) {
        for (uint8_t i = 0; i < nConfigCount; i++)
        {
            if (strcmp(nConfig[i].Name, name) == 0) {
                if (strcmp(nConfig[i].Value, value) == 0) {
                    nconfigEqualsArray[i] = true;

                    break;
                }
            }
        }

        return ResponsePendingExtra;
    }

    return ResponseError;
}

bool Sodaq_nbIOT::attachGprs(uint32_t timeout)
{
    uint32_t start = millis();
    uint32_t delay_count = 500;

    while (!is_timedout(start, timeout)) {
        if (isConnected()) {
            return true;
        }
        //println("AT+CGATT=1");

        //if (readResponse() == ResponseOK) {
        //    return true;
        //}

        sodaq_wdt_safe_delay(delay_count);

        // Next time wait a little longer, but not longer than 5 seconds
        if (delay_count < 5000) {
            delay_count += 1000;
        }
    }

    return false;
}

//int Sodaq_nbIOT::createSocket(uint16_t localPort)
//{
//    // only Datagram/UDP is supported
//    print("AT+NSOCR=DGRAM,17,");
//    print(localPort);
//    print(",1"); // enable incoming message URC (NSONMI)
//
//    uint8_t socket;
//
//    if (readResponse<uint8_t, uint8_t>(_createSocketParser, &socket, NULL) == ResponseOK) {
//        return socket;
//    }
//
//    return SOCKET_FAIL;
//}
//
//ResponseTypes Sodaq_nbIOT::_createSocketParser(ResponseTypes& response, const char* buffer, size_t size,
//        uint8_t* socket, uint8_t* dummy)
//{
//    if (!socket) {
//        return ResponseError;
//    }
//
//    int value;
//
//    if (sscanf(buffer, "%d", &value) == 1) {
//        *socket = value;
//
//        return ResponseEmpty;
//    }
//
//    return ResponseError;
//}

//bool connectSocket(uint8_t socket, const char* host, uint16_t port)
//{
//    return false;
//}

// Disconnects the modem from the network.
bool Sodaq_nbIOT::disconnect()
{
    println("AT+CGATT=0");

    return (readResponse(NULL, 40000) == ResponseOK);
}

// Returns true if the modem is connected to the network and has an activated data connection.
bool Sodaq_nbIOT::isConnected()
{
    uint8_t value = 0;

    println("AT+CGATT?");

    if (readResponse<uint8_t, uint8_t>(_cgattParser, &value, NULL) == ResponseOK) {
        return (value == 1);
    }

    return false;
}

// Gets the Received Signal Strength Indication in dBm and Bit Error Rate.
// Returns true if successful.
bool Sodaq_nbIOT::getRSSIAndBER(int8_t* rssi, uint8_t* ber)
{
    static char berValues[] = { 49, 43, 37, 25, 19, 13, 7, 0 }; // 3GPP TS 45.008 [20] subclause 8.2.4

    println("AT+CSQ");

    int csqRaw = 0;
    int berRaw = 0;

    if (readResponse<int, int>(_csqParser, &csqRaw, &berRaw) == ResponseOK) {
        *rssi = ((csqRaw == 99) ? 0 : convertCSQ2RSSI(csqRaw));
        *ber = ((berRaw == 99 || static_cast<size_t>(berRaw) >= sizeof(berValues)) ? 0 : berValues[berRaw]);

        return true;
    }

    return false;
}

/*
    The range is the following:
    0: -113 dBm or less
    1: -111 dBm
    2..30: from -109 to -53 dBm with 2 dBm steps
    31: -51 dBm or greater
    99: not known or not detectable or currently not available
*/
int8_t Sodaq_nbIOT::convertCSQ2RSSI(uint8_t csq) const
{
    return -113 + 2 * csq;
}

uint8_t Sodaq_nbIOT::convertRSSI2CSQ(int8_t rssi) const
{
    return (rssi + 113) / 2;
}

bool Sodaq_nbIOT::startsWith(const char* pre, const char* str)
{
    return (strncmp(pre, str, strlen(pre)) == 0);
}

size_t Sodaq_nbIOT::ipToString(IP_t ip, char* buffer, size_t size)
{
    return snprintf(buffer, size, IP_FORMAT, IP_TO_TUPLE(ip));
}

bool Sodaq_nbIOT::isValidIPv4(const char* str)
{
    uint8_t segs = 0; // Segment count
    uint8_t chcnt = 0; // Character count within segment
    uint8_t accum = 0; // Accumulator for segment

    if (!str) {
        return false;
    }

    // Process every character in string
    while (*str != '\0') {
        // Segment changeover
        if (*str == '.') {
            // Must have some digits in segment
            if (chcnt == 0) {
                return false;
            }

            // Limit number of segments
            if (++segs == 4) {
                return false;
            }

            // Reset segment values and restart loop
            chcnt = accum = 0;
            str++;
            continue;
        }

        // Check numeric
        if ((*str < '0') || (*str > '9')) {
            return false;
        }

        // Accumulate and check segment
        if ((accum = accum * 10 + *str - '0') > 255) {
            return false;
        }

        // Advance other segment specific stuff and continue loop
        chcnt++;
        str++;
    }

    // Check enough segments and enough characters in last segment
    if (segs != 3) {
        return false;
    }

    if (chcnt == 0) {
        return false;
    }

    // Address OK

    return true;
}

bool Sodaq_nbIOT::waitForSignalQuality(uint32_t timeout)
{
    uint32_t start = millis();
    const int8_t minRSSI = getMinRSSI();
    int8_t rssi;
    uint8_t ber;

    uint32_t delay_count = 500;

    while (!is_timedout(start, timeout)) {
        if (getRSSIAndBER(&rssi, &ber)) {
            if (rssi != 0 && rssi >= minRSSI) {
                _lastRSSI = rssi;
                _CSQtime = (int32_t)(millis() - start) / 1000;
                return true;
            }
        }

        sodaq_wdt_safe_delay(delay_count);

        // Next time wait a little longer, but not longer than 5 seconds
        if (delay_count < 5000) {
            delay_count += 1000;
        }
    }

    return false;
}

ResponseTypes Sodaq_nbIOT::_cgattParser(ResponseTypes& response, const char* buffer, size_t size, uint8_t* result, uint8_t* dummy)
{
    if (!result) {
        return ResponseError;
    }

    int val;
    if (sscanf(buffer, "+CGATT: %d", &val) == 1) {
        *result = val;
        return ResponseEmpty;
    }

    return ResponseError;
}

ResponseTypes Sodaq_nbIOT::_csqParser(ResponseTypes& response, const char* buffer, size_t size,
                                      int* rssi, int* ber)
{
    if (!rssi || !ber) {
        return ResponseError;
    }

    if (sscanf(buffer, "+CSQ: %d,%d", rssi, ber) == 2) {
        return ResponseEmpty;
    }

    return ResponseError;
}

bool Sodaq_nbIOT::sendMessage(const uint8_t* buffer, size_t size)
{
    if (size > 512) {
        return false;
    }

    print("AT+NMGS=");
    print(size);
    print(",\"");

    for (uint16_t i = 0; i < size; ++i) {
        print(static_cast<char>(NIBBLE_TO_HEX_CHAR(HIGH_NIBBLE(buffer[i]))));
        print(static_cast<char>(NIBBLE_TO_HEX_CHAR(LOW_NIBBLE(buffer[i]))));
    }

    println("\"");

    return (readResponse() == ResponseOK);
}

int Sodaq_nbIOT::getSentMessagesCount(SentMessageStatus filter)
{
    println("AT+NQMGS");

    uint16_t pendingCount = 0;
    uint16_t errorCount = 0;

    if (readResponse<uint16_t, uint16_t>(_nqmgsParser, &pendingCount, &errorCount) == ResponseOK) {
        if (filter == Pending) {
            return pendingCount;
        }
        else if (filter == Error) {
            return errorCount;
        }
    }

    return -1;
}

ResponseTypes Sodaq_nbIOT::_nqmgsParser(ResponseTypes& response, const char* buffer, size_t size, uint16_t* pendingCount, uint16_t* errorCount)
{
    if (!pendingCount || !errorCount) {
        return ResponseError;
    }

    int pendingValue;
    int errorValue;

    if (sscanf(buffer, "PENDING=%d,SENT=%*d,ERROR=%d", &pendingValue, &errorValue) == 2) {
        *pendingCount = pendingValue;
        *errorCount = errorValue;

        return ResponseEmpty;
    }

    return ResponseError;
}

bool Sodaq_nbIOT::sendMessage(const char* str)
{
    return sendMessage((const uint8_t*)str, strlen(str));
}

bool Sodaq_nbIOT::sendMessage(String str)
{
    return sendMessage(str.c_str());
}

// ==============================
// on/off class
// ==============================
Sodaq_nbIotOnOff::Sodaq_nbIotOnOff()
{
    _onoffPin = -1;
    _onoff_status = false;
}

// Initializes the instance
void Sodaq_nbIotOnOff::init(int onoffPin)
{
    if (onoffPin >= 0) {
        _onoffPin = onoffPin;
        // First write the output value, and only then set the output mode.
        digitalWrite(_onoffPin, LOW);
        pinMode(_onoffPin, OUTPUT);
    }
}

void Sodaq_nbIotOnOff::on()
{
    if (_onoffPin >= 0) {
        digitalWrite(_onoffPin, HIGH);
    }

    _onoff_status = true;
}

void Sodaq_nbIotOnOff::off()
{
    // The GPRSbee is switched off immediately
    if (_onoffPin >= 0) {
        digitalWrite(_onoffPin, LOW);
    }

    // Should be instant
    // Let's wait a little, but not too long
    delay(50);
    _onoff_status = false;
}

bool Sodaq_nbIotOnOff::isOn()
{
#if defined(ARDUINO_ARCH_AVR)
    // Use the onoff pin, which is close to useless
    bool status = digitalRead(_onoffPin);
    return status;
#elif defined(ARDUINO_ARCH_SAMD)
    // There is no status pin. On SAMD we cannot read back the onoff pin.
    // So, our own status is all we have.
    return _onoff_status;
#endif

    // Let's assume it is on.
    return true;
}

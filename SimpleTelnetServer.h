/*
    Telnet support for the ESP8266 Wifi.
    Copyright (c) 2016 Kenneth S. Davis, All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Extends supports of TelnetServer base class:

    RFC 857 - TELNET ECHO OPTION
    RFC 858 - TELNET SUPPRESS GO AHEAD OPTION
    RFC 1079 - TELNET TERMINAL SPEED OPTION
*/



#ifndef _SIMPLETELNETSERVER_h
#define _SIMPLETELNETSERVER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Telnet.h"

// telnet options
#define TELNET_OPTION_TERMINAL_SPEED    32

// Future work
//#define TELNET_OPTION_COM_PORT          44

/*
// Future work
#define TELNET_COM_PORT_OPTION_SIG
#define TELNET_COM_PORT_SET_BAUDRATE        "1"
#define TELNET_COM_PORT_SET_DATASIZE        "2"
#define TELNET_COM_PORT_SET_PARITY          "3"
#define TELNET_COM_PORT_SET_STOPSIZE        "4"
#define TELNET_COM_PORT_SET_CONTROL         "5"
#define TELNET_COM_PORT_NOTFY_LINESTATE     "6"
#define TELNET_COM_PORT_NOTFY_MODEMSTATE    "7"
#define TELNET_COM_PORT_FLOW_SUSPEND        "8"
#define TELNET_COM_PORT_FLOW_RESUME         "9"
#define TELNET_COM_PORT_LINE_MASK           "10"
#define TELNET_COM_PORT_MODEM_MASK          "11"
#define TELNET_COM_PORT_PURGE               "12"
*/

class SimpleTelnetServer : public TelnetServer
{
public:

    SimpleTelnetServer();

    virtual ~SimpleTelnetServer();

    uint8_t recvBuffer[1024];
    uint8_t recvBufLen;

protected:

    virtual bool _processSubNegotiation(WiFiClient &client, struct ClientStruct &str);

    virtual bool _processOption(WiFiClient &client, struct ClientStruct &str);
};

#endif


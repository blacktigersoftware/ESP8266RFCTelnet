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

    Currently supports:

        RFC 854 - TELNET PROTOCOL SPEFICICATIONS
        RFC 855 - TELNET OPTION SPECIFICATIONS
        RFC 856 - TELNET BINARY TRANSMISSION

    RFC 854 is the key spec, as it allows for extensible support, which
    is represented by two virtual functions that can be handled.


    This is a base class that receives incoming bytes from a WiFi client
    and decodes the protocol.   
*/

#ifndef _TELNET_h
#define _TELNET_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <WiFiServer.h>
#include <WiFiClient.h>

#define TELNET_SE   240
#define TELNET_NOP  241
#define TELNET_DM   242
#define TELNET_BRK  243
#define TELNET_IP   244
#define TELNET_AO   245
#define TELNET_AYT  246
#define TELNET_EC   247
#define TELNET_EL   248
#define TELNET_GA   249
#define TELNET_SB   250
#define TELNET_WILL 251
#define TELNET_WONT 252
#define TELNET_DO   253
#define TELNET_DONT 254
#define TELNET_IAC  255

// telnet options
#define TELNET_OPTION_TRANSMIT_BINARY   0
#define TELNET_OPTION_ECHO              1
#define TELNET_OPTION_SUPPRESS_GA       3

class TelnetServer
{
public:

    void begin();
    void end();

    void handleClient();

    virtual ~TelnetServer();

protected:

    TelnetServer();

    TelnetServer(int port);

    enum ClientState
    {
        Normal,
        InTelnetOpt0,
        InTelnetOpt1,
        InTelnetSubNego0,
        InTelnetSubNego1
    };

    // currently we only support one client, however,
    // should the need arise to support multiple clients
    // each client should have one of these.
    struct ClientStruct
    {
        enum ClientState clientState;
        byte            opt0;
        byte            opt1;
        byte            echo;
        byte            noga;

        uint8_t         negoBuffer[128];
        uint8_t         negoBufferLen;

        uint8_t         buffer[1024];
        uint8_t         bufferLen;
    };


    // on 'false' the subnegotiation was not handled
    virtual bool _processSubNegotiation(WiFiClient &client, struct ClientStruct &str);

    /*
        on 'false' the option was not processed,
        for str.clientState of 
            Normal:       opt0 is the incoming byte
            InTelnetOpt0: opt0 is the command
            InTelnetOpt1: opt0 is "DO/DONT/WILL/WONT", opt1 is the option
    */
    virtual bool _processOption(WiFiClient &client, struct ClientStruct &str);

    /*
        initializes the client struct
    */
    static void _initClient(struct ClientStruct &str);

    /* our server */
    WiFiServer _server;

    /* our single client -- This can be easy extended to support multiple
       clients --- but, consider.  What would happen say if one client turned
       on a motor and another client turned off a motor?  Does a firmware board
       really need to support multiple clients?  However, nothing prevents the 
       multiple servers running on different ports for different reasons....
    */

    WiFiClient _client;

    /* holds are client data, should we handle multiple clients */
    struct ClientStruct _clientStr;
};

#endif


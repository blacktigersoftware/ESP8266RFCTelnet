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

#include "Telnet.h"

//#define DEBUG_TELNET  Serial

TelnetServer::TelnetServer(int port) :
    _server(port)
{
}

TelnetServer::TelnetServer() :
    _server(23)
{
}

TelnetServer::~TelnetServer()
{
    end();
}

void TelnetServer::begin()
{
#ifdef DEBUG_TELNET
    DEBUG_TELNET.println("Telnet server started");
#endif

    _server.begin();
}

void TelnetServer::end()
{
    if (_client)
        _client.stop();

    _server.close();
}

void TelnetServer::handleClient()
{
    // are we running?
    if (_server.status() == CLOSED)
        return;

    // new client?
    if (_server.hasClient())
    {
        if (_client)
        {
            // already have one, sorry
#ifdef DEBUG_TELNET
            DEBUG_TELNET.println("Rejecting client");
#endif
            WiFiClient rejectClient = _server.available();
            rejectClient.stop();
        }
        else
        {
            // grab the new client
            _client = _server.available();
            _initClient(_clientStr);
#ifdef DEBUG_TELNET
            DEBUG_TELNET.println("Accepted new client");
#endif
        }
    }

    // so, do we still have a client?
    if (_client)
    {
        // and is it connected?
        if (!_client.connected())
        {
#ifdef DEBUG_TELNET
            DEBUG_TELNET.println("Existing client stopped");
#endif
            _client.stop();
            return;
        }

        // at this point, we have a client and it is connected
        while (_client.available())
        {
            char c = _client.read();
#ifdef DEBUG_TELNET
            DEBUG_TELNET.print(c, HEX);
            DEBUG_TELNET.print(" ");
#endif
            switch (_clientStr.clientState)
            {
                case Normal:
                {
                    // don't pass this to sub-classes, just advance state
                    if (c == TELNET_IAC)
                    {
                        _clientStr.clientState = InTelnetOpt0;
                        return;
                    }

                    _clientStr.opt0 = c;
                    _processOption(_client, _clientStr);
                    return;
                }

                // Control
                case InTelnetOpt0:
                {
                    _clientStr.opt0 = c;
                    switch (c)
                    {
                        // don't pass this to sub-classes, just advance state
                        case TELNET_WILL:
                        case TELNET_DO:
                        case TELNET_WONT:
                        case TELNET_DONT:
                        {
                            _clientStr.clientState = InTelnetOpt1;
                            return;
                        }

                        case TELNET_IAC:
                        {
                            // this is an escaped 0xff, go back to normal mode
                            // and send to sub-classes as just a normal 0xff char
                            _clientStr.clientState = Normal;
                            _clientStr.opt0 = c;
                            _processOption(_client, _clientStr);
                            return;
                        }
                        case TELNET_SB: // start of sub-nego
                        {
                            // don't pass this to sub-classes, just advance state
                            _clientStr.clientState = InTelnetSubNego0;
                            _clientStr.negoBufferLen = 0;
                            return;
                        }

                        default:
                        {
                            // got one of these really, really old IAC commands
                            // for EL, EC, GA, etc, pass to client.  Regardless
                            // change back to normal mode.
                            _processOption(_client, _clientStr);
                            _clientStr.clientState = Normal;
                            return;
                        }
                    }
                }

                // Option
                case InTelnetOpt1:
                {
                    _clientStr.opt1 = c;

                    // here, we let the sub-class determine handling of TELNET options
                    // If it handles it, we are done, otherwise we send appropriate DONT
                    // WONT.  And back to normal mode.
                    if (_processOption(_client, _clientStr))
                    {
                        _clientStr.clientState = Normal;
                        return;
                    }

                    // default handling for unhandled requests
                    if (_clientStr.opt0 == TELNET_WILL)
                    {
                        // send DONT
                        _clientStr.buffer[_clientStr.bufferLen] = TELNET_IAC;
                        _clientStr.bufferLen++;
                        _clientStr.buffer[_clientStr.bufferLen] = TELNET_DONT;
                        _clientStr.bufferLen++;
                        _clientStr.buffer[_clientStr.bufferLen] = c;
                        _clientStr.bufferLen++;

                    }
                    else if (_clientStr.opt0 == TELNET_DO)
                    {
                        // send WONT
                        _clientStr.buffer[_clientStr.bufferLen] = TELNET_IAC;
                        _clientStr.bufferLen++;
                        _clientStr.buffer[_clientStr.bufferLen] = TELNET_WONT;
                        _clientStr.bufferLen++;
                        _clientStr.buffer[_clientStr.bufferLen] = c;
                        _clientStr.bufferLen++;
                    }
                    else
                    {
                        // something weird this way comes.. :<
#ifdef DEBUG_TELNET
                        DEBUG_TELNET.println("Unspecified opt");
#endif
                    }

                    _clientStr.clientState = Normal;
                    return;
                }

                // Sub Negoiations
                case InTelnetSubNego0:
                {
                    // In order to handle an embedded 0xff in subnegotiation mode,
                    // we get an extra state, InTelnetSubNego1.
                    if (c == TELNET_IAC)
                        _clientStr.clientState = InTelnetSubNego1;
                    else
                    {
                        _clientStr.negoBuffer[_clientStr.negoBufferLen] = c;
                        _clientStr.negoBufferLen++;
                    }
                    break;
                }

                case InTelnetSubNego1:
                {
                    // in this state, we either get a second TELNET_IAC or otherwise
                    // we should get the TELNET_SE.
                    if (c == TELNET_IAC)
                    {
                        // they sent an esc'd 0xff, back to InTelnetSubNego0
                        _clientStr.clientState = InTelnetSubNego0;
                        _clientStr.negoBuffer[_clientStr.negoBufferLen] = TELNET_IAC;
                        _clientStr.negoBufferLen++;
                    }
                    else if (c == TELNET_SE)
                    {
                        // we got the completed subnegotiation, process it
                        _processSubNegotiation(_client, _clientStr);
                        _clientStr.clientState = Normal;
                    }
                    else
                    {
#ifdef DEBUG_TELNET
                        DEBUG_TELNET.println("");
                        DEBUG_TELNET.print("Unknown subneg option:");
                        DEBUG_TELNET.println(c, HEX);
#endif
                    }

                    break;
                }
            }
        }

        // Send outbound data
        if (_clientStr.bufferLen > 0)
        {
#ifdef DEBUG_TELNET
            DEBUG_TELNET.println("");
            DEBUG_TELNET.print("Sending bytes: ");
            DEBUG_TELNET.println(_clientStr.bufferLen);

            for (auto i = 0; i < _clientStr.bufferLen; i++)
            {
                DEBUG_TELNET.print(_clientStr.buffer[i], HEX);
                DEBUG_TELNET.print(" ");
            }
            DEBUG_TELNET.println();
#endif

            _client.write(&_clientStr.buffer[0], _clientStr.bufferLen);
            _clientStr.bufferLen = 0;
        }
    }
}

void TelnetServer::_initClient(ClientStruct& str)
{
    str.clientState = Normal;
    str.echo = 0;
    str.negoBufferLen = 0;
    str.bufferLen = 0;

    str.opt0 = 0;
    str.opt1 = 0;
}

bool TelnetServer::_processOption(WiFiClient& client, ClientStruct& str)
{
    if (str.clientState == InTelnetOpt0)
    {
        return false;
    }

    if (str.clientState == InTelnetOpt1)
    {
        switch (str.opt1)
        {
            case TELNET_OPTION_TRANSMIT_BINARY:
            case TELNET_OPTION_ECHO:
            case TELNET_OPTION_SUPPRESS_GA:
            {
                if (str.opt0 == TELNET_WILL)
                {
                    str.buffer[str.bufferLen] = TELNET_IAC;
                    str.bufferLen++;
                    str.buffer[str.bufferLen] = TELNET_DO;
                    str.bufferLen++;
                    str.buffer[str.bufferLen] = str.opt1;
                    str.bufferLen++;
                }

                else if (_clientStr.opt0 == TELNET_DO)
                {
                    str.buffer[str.bufferLen] = TELNET_IAC;
                    str.bufferLen++;
                    str.buffer[str.bufferLen] = TELNET_WILL;
                    str.bufferLen++;
                    str.buffer[str.bufferLen] = str.opt1;
                    str.bufferLen++;
                }

                if (str.opt1 == TELNET_OPTION_ECHO)
                {
    #ifdef DEBUG_TELNET
                    DEBUG_TELNET.println("Echo turned on");
    #endif
                    str.echo = 1;
                }
                if (str.opt1 == TELNET_OPTION_SUPPRESS_GA)
                {
    #ifdef DEBUG_TELNET
                    DEBUG_TELNET.println("GA suppressed");
    #endif
                    str.noga = 1;
                }

                str.clientState = Normal;
                return true;
            }
            default:
                return false;
        }
    }

    return false;
}

bool TelnetServer::_processSubNegotiation(WiFiClient &client, struct ClientStruct &str)
{
    return false;
}

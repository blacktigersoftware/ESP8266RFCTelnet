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

    RFC 1079 - TELNET TERMINAL SPEED OPTION
*/

#include "SimpleTelnetServer.h"

SimpleTelnetServer::SimpleTelnetServer()
{
    recvBufLen = 0;
}

SimpleTelnetServer::~SimpleTelnetServer()
{
    end();
}

/*
    This example extends TelnetServer with additional RFC's.
*/
bool SimpleTelnetServer::_processOption(WiFiClient& client, ClientStruct& str)
{
    // check with the base class first.....
    if (TelnetServer::_processOption(client, str))
        return true;

    if (str.clientState == Normal)
    {
        if (str.echo)
        {
            str.buffer[str.bufferLen] = str.opt0;
            str.bufferLen++;
        }

        recvBuffer[recvBufLen] = str.opt0;
        recvBufLen++;

        return true;
    }

    if (str.clientState == InTelnetOpt0)
    {
        switch (str.opt0)
        {
            case TELNET_AYT:
            {

                // send a '.'
                str.buffer[str.bufferLen] = '.';
                str.bufferLen++;
                str.clientState = Normal;
                return true;
            }

            case TELNET_BRK: // break cmd
            case TELNET_AO: // abort output
            case TELNET_NOP: // Nop
            case TELNET_DM: // Data mark
            case TELNET_GA: // Go-ahead flow-control
            {

    #ifdef DEBUG_TELNET
                DEBUG_TELNET.print("Unsupported telnet option:");
                DEBUG_TELNET.println(str.opt0, HEX);
    #endif
                return false;
            }
            case TELNET_EC: // erase last character
            {
                if (str.echo)
                {
                    if (str.buffer[str.bufferLen] > 0)
                    {
                        str.buffer[str.bufferLen] = 0;
                        str.bufferLen--;
                    }
                }

                _clientStr.clientState = Normal;
                return true;
            }
            case TELNET_EL: // erase current line
            {
    #ifdef DEBUG_TELNET
                DEBUG_TELNET.print("Unsupported telnet option:");
                DEBUG_TELNET.println(str.opt0, HEX);
    #endif
                _clientStr.clientState = Normal;
                return true;
            }
            case TELNET_IP: // interrupt process ---- what to do...?
            {
    #ifdef DEBUG_TELNET
                DEBUG_TELNET.print("Unsupported telnet option:");
                DEBUG_TELNET.println(str.opt0, HEX);
    #endif
                _clientStr.clientState = Normal;
                return true;
            }
            default:
            {
    #ifdef DEBUG_TELNET
                DEBUG_TELNET.print("Unknown telnet option:");
                DEBUG_TELNET.println(str.opt0, HEX);
    #endif
                return false;
            }
        }
    }

    if (str.clientState == InTelnetOpt1)
    {
        switch (str.opt1)
        {
            // extend for RFC 1079 - TELNET TERMINAL SPEED OPTION, also needed
            // to add subnegotiation support for this.
            case TELNET_OPTION_TERMINAL_SPEED:
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

                str.clientState = Normal;
                return true;
            }

            default:
            {
                return false;
            }
        }
    }

    return false;
}

bool SimpleTelnetServer::_processSubNegotiation(WiFiClient &client, struct ClientStruct &str)
{
    // we have captured a sb block:
    // IAC SB [ captured ] IAC SE
    if (TelnetServer::_processSubNegotiation(client, str))
        return true;

    switch (str.negoBuffer[0])
    {
        case TELNET_OPTION_TERMINAL_SPEED:
        {
            int txSpeed = 9600;
            int rxSpeed = 9600;

            if (str.negoBuffer[1] == 1)
            {
                str.buffer[str.bufferLen++] = TELNET_IAC;
                str.buffer[str.bufferLen++] = TELNET_SB;
                str.buffer[str.bufferLen++] = TELNET_OPTION_TERMINAL_SPEED;
                str.buffer[str.bufferLen++] = 0;
                str.bufferLen += sprintf((char*)&str.buffer[str.bufferLen], "%d,%d", txSpeed, rxSpeed);
                str.buffer[str.bufferLen++] = TELNET_IAC;
                str.buffer[str.bufferLen] = TELNET_SE;

    #ifdef DEBUG_TELNET
                DEBUG_TELNET.println("SB TERMINAL SPEED");
                for (int i = 0; i < str.bufferLen; i++)
                {
                    DEBUG_TELNET.print(str.buffer[i], HEX);
                    DEBUG_TELNET.print(" ");
                }
                DEBUG_TELNET.println();
    #endif
                str.negoBufferLen = 0;
                return true;
            }

            break;
        }

        default:
            return false;
    }

    return false;
}

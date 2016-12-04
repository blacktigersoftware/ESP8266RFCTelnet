#include <ESP8266WiFi.h>
#include <SimpleTelnetServer.h>
#include <Telnet.h>

const char* ssid = "**********";
const char* password = "**********";

SimpleTelnetServer Telnet;

void setup() {
  Serial1.begin(115200);
  WiFi.begin(ssid, password);
  Serial1.print("\nConnecting to "); Serial1.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial1.print("Could not connect to"); Serial1.println(ssid);
    while(1) delay(500);
  }
  //start UART and the server
  Serial.begin(115200);

  Telnet.begin();
  
  Serial1.print("Ready! Use 'telnet ");
  Serial1.print(WiFi.localIP());
  Serial1.println(" 23' to connect");
}

void loop() {

    Telnet.handleClient();

    /*  You are responsible to reading and clearing the Telnet.recvBuffer
     *  and setting the Telnet.recvBufLen.  Typically you may
     *  want to scan for '\n' and process lines at a time.
     *  
     *  I'm going to update/change the buffer behavior to use a circular
     *  buffer in the near future.
     */
    if (Telnet.recvBufLen > 0)
    {
        Serial.print("Received: ");
        for (auto i = 0; i < Telnet.recvBufLen; i++)
            Serial.print(Telnet.recvBuffer[i], HEX);
        Serial.println("");
        Telnet.recvBufLen = 0;
    }

    delay(10);


}

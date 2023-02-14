/**
   Gopro Router for esp32

   It communicates with a second esp32 using the UART 2 in this device and UART 0 in the destination
   Documentation on the pins used for UART taken from:https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/
   Example of the configuration of UART 2 taken from https://github.com/G6EJD/ESP32-Using-Hardware-Serial-Ports/blob/master/ESP32_Using_Serial2.ino

*/
#include "SerialTransfer.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
#define RXD2 16
#define TXD2 17
const char *ssid = "Unknown";
const char *password = "daredevilme";
bool activeReceive = false;
bool binaryStarted = false;
int currentPacket = 0;
WebServer server(80);
SerialTransfer myTransfer;
struct Message {
  char command;
  float value;
  char strValue[30];
} message;
struct ImageChunk {
  int packetType;
  int packetId;
  int bytesInPacket;
  int totalPackets;
  unsigned char chunk[128];
} imageChunk;
String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".JPG")) return "image/jpeg";
  else if (filename.endsWith(".JPEG")) return "image/jpeg";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  //Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type

  /*if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server->streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
    }
    //Serial.println("\tFile Not Found");
    return false; */                                        // If the file doesn't exist, return false
  return true;
}
void connectCamera() {
  String out;
  out.reserve(2600);
  char temp[70];
  out += "Connecting ..";
  message.command = 'c';
  String someValue = "connecting..";
  someValue.toCharArray(message.strValue, someValue.length() + 1);
  myTransfer.sendDatum(message);
  server.send(200, "application/json", out);
}
void disconnectCamera() {
  String out;
  out.reserve(2600);
  char temp[70];
  out += "Connecting ..";
  Serial2.println('X');
  server.send(200, "application/json", out);
}
unsigned int downloadImageTimer=0;
void resetTransmission(){
    activeReceive=false;
    server.send(200, "image/jpeg");
    Serial.println("end of transmission");
    currentPacket = 0;
  }
void setup(void) {
  //this pin controls if we are receiving a text data or binary data
  // when on LOW it means binary data is being requested
  pinMode(27, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("GOPRO ROUTER");
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  myTransfer.begin(Serial2);
  Serial.println("Serial Txd is on pin: " + String(TX));
  Serial.println("Serial Rxd is on pin: " + String(RX));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("mygopro")) {
    Serial.println("MDNS responder started");
  }

  server.on(F("/"), []() {
    server.send(200, "text/plain", "hello from esp32!");
  });

  server.on(UriBraces("/users/{}"), []() {
    String user = server.pathArg(0);
    server.send(200, "text/plain", "User: '" + user + "'");
  });

  server.on("/connect", connectCamera);
  server.on("/disconnect", disconnectCamera);
  
  server.on(UriBraces("/pics/{}"), []() {
    String jpgImage = server.pathArg(0);
    downloadImageTimer=millis();
    activeReceive = true;
    currentPacket = 0;
    Serial.println("Attemping picture");
    
    message.command = 't';
    //String someValue = "GOPR4256.JPG";
    jpgImage.toCharArray(message.strValue, jpgImage.length() + 1);
    myTransfer.sendDatum(message);
    while (activeReceive) {
      if(millis()>(downloadImageTimer+7000) && currentPacket == 0){
        resetTransmission();
      }
      if(myTransfer.available()){
        myTransfer.rxObj(imageChunk);
        if (currentPacket == 0) {
          Serial.print("bytes:");
          Serial.println(imageChunk.bytesInPacket);
          //server.setContentLength(17952);
          server.setContentLength(imageChunk.bytesInPacket);
        }
        Serial.print(currentPacket);
        Serial.print("/");
        Serial.print(imageChunk.packetId);
        Serial.print("/");
        Serial.println(imageChunk.totalPackets);
        //now transferring to the client
        //uint8_t buff[128] = { 0 };
        //memcpy(buff, imageChunk.chunk, sizeof(imageChunk.chunk));
        /*for (int j = 0; j < sizeof(imageChunk.chunk); j++) {
          server.send(imageChunk.chunk[j]);
        }*/
        WiFiClient client = server.client();
        client.write(imageChunk.chunk,sizeof(imageChunk.chunk));
        //server.send(imageChunk.chunk);
        currentPacket = currentPacket + 1;
        if ((currentPacket > imageChunk.totalPackets) || (imageChunk.packetId > imageChunk.totalPackets) || (imageChunk.packetType==2)) {//packetType 2 means error
          resetTransmission();
        }
      }
      yield();
    }
    });

    server.on(UriBraces("/files/{}"), []() {
    String jpgImage = server.pathArg(0);
    downloadImageTimer=millis();
    activeReceive = true;
    currentPacket = 0;
    Serial.println("Attemping picture");
    
    message.command = 'f';
    //String someValue = "GOPR4256.JPG";
    jpgImage.toCharArray(message.strValue, jpgImage.length() + 1);
    myTransfer.sendDatum(message);
    while (activeReceive) {
      if(millis()>(downloadImageTimer+7000) && currentPacket == 0){
        resetTransmission();
      }
      if(myTransfer.available()){
        myTransfer.rxObj(imageChunk);
        if (currentPacket == 0) {
          Serial.print("bytes:");
          Serial.println(imageChunk.bytesInPacket);
          server.setContentLength(imageChunk.bytesInPacket);
        }
        Serial.print(currentPacket);
        Serial.print("/");
        Serial.print(imageChunk.packetId);
        Serial.print("/");
        Serial.println(imageChunk.totalPackets);
        WiFiClient client = server.client();
        client.write(imageChunk.chunk,sizeof(imageChunk.chunk));
        //server.send(imageChunk.chunk);
        currentPacket = currentPacket + 1;
        if ((currentPacket > imageChunk.totalPackets) || (imageChunk.packetId > imageChunk.totalPackets) || (imageChunk.packetType==2)) {//packetType 2 means error
          resetTransmission();
        }
      }
      yield();
    }
    });

    server.on(UriRegex("^\\/users\\/([0-9]+)\\/devices\\/([0-9]+)$"), []() {
      String user = server.pathArg(0);
      String device = server.pathArg(1);
      server.send(200, "text/plain", "User: '" + user + "' and Device: '" + device + "'");
    });

    server.begin();
    Serial.println("HTTP server started");
  }

  void loop(void) {
    server.handleClient();

  }
  //this section is intended for receiving the messages from the esp32 connected directly to the gopro
  /*while (Serial2.available() && WiFi.status() == WL_CONNECTED) {
    //Serial.print(char(Serial2.read()));
    if(digitalRead(27)==HIGH && activeReceive == true){
      Serial.println("finish receiving...");
      //this is a control state in which
      // we check if the previous state was sending binary data
      //in which case we close with a sendstatus
      if(binaryStarted == true){
        binaryStarted = false;
        activeReceive=false;
        //server.send(200, "image/jpeg");
      }
    } else if(digitalRead(27)==LOW && activeReceive == true){
      // we are receving binary data
      Serial.println("receiving...");
      server.send(Serial2.read());
      if(binaryStarted == false){
        binaryStarted = true;
      }
    } else {
      String incomingString = Serial2.readStringUntil('\n');
      Serial.println(incomingString);
    }
    }*/

  /*if (Serial.available() > 0 && WiFi.status() != WL_CONNECTED)
    {
    }

    if(incomingString.indexOf(">>>") != -1 && activeReceive == true){
      //ignore this delimiter
    } else if(incomingString.indexOf("<<<") != -1 && activeReceive == true){
      // close the entire connection with a send

      activeReceive = false;
    } else if(activeReceive == true){
      //unsigned char output[200];
      //size_t outlen;
      //mbedtls_base64_decode(output, 200, &outlen, incomingString, strlen(incomingString));
      //server.sendContent(incomingString);

    }
    }*/

//}

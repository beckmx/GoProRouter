#include <ArduinoJson.h>
#include <GoProControl.h>
#include "SerialTransfer.h"


SerialTransfer myTransfer;
struct STRUCT {
  char command;
  float value;
  char strValue[30];
} message;
// Allocate the JSON document
StaticJsonDocument<4000> status;
StaticJsonDocument<5000> media; // media list
//GO PRO mac 6:41:69:92:85:B5
GoProControl gp("Unknown HERO7 Black", "3d3-snv-GRX", HERO7);

void setup()
{
  
  // this is the pin that indicates if the data is binary
  //or plain text
  Serial.begin(115200);
  #if defined(ARDUINO_ARCH_ESP32)
  Serial.println("Extractor of images");  
  gp.enableDebug(&Serial);
  pinMode(27, OUTPUT);
  digitalWrite(27,HIGH);
  myTransfer.begin(Serial);
  gp.enableTransfer(&myTransfer);
  #elif defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
  Serial1.setRX(1);
  Serial1.setTX(0);
  Serial1.begin(115200);
  Serial.println("GOPRO Direct");  
  gp.enableDebug(&Serial1);
  myTransfer.begin(Serial1);
  gp.enableTransfer(&myTransfer);
  pinMode(2, OUTPUT);
  digitalWrite(2,HIGH);
  #endif
}

void compute_status()
{
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(status, gp.getStatus());

  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    #if defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    #endif
    return;
  }

  // Print values
  // To understand these values see the camera status file, for example for MAX:
  // https://github.com/KonradIT/goprowifihack/blob/master/MAX/CameraStatus.md
  byte battery = status["status"]["1"];
  byte charge = status["status"]["2"];
  byte mode = status["settings"]["1"];
  byte vr = status["settings"]["2"];

  Serial.print("Battery: ");
  Serial.println(battery);
  Serial.print("Charge: ");
  Serial.println(charge);
  Serial.print("Mode: ");
  Serial.println(mode);
  Serial.print("Video Resolution: ");
  Serial.println(vr);
  #if defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
    Serial1.print("Battery: ");
  Serial1.println(battery);
  Serial1.print("Charge: ");
  Serial1.println(charge);
  Serial1.print("Mode: ");
  Serial1.println(mode);
  Serial1.print("Video Resolution: ");
  Serial1.println(vr);
    #endif
}

void compute_mediaList()
{
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(media, gp.getMediaList());

  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    #if defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    #endif
    return;
  }

  // Print values.
  const char *id = media["id"];
  const char *folder = media["media"][0]["d"];
  JsonArray array = media["media"][0]["fs"];
  byte pic = array.size();

  Serial.print("ID: ");
  Serial.println(id);
  Serial.print("Folder: ");
  Serial.println(folder);
  Serial.print("Number of pic: ");
  Serial.println(pic);
  for(int iindex=0;iindex<array.size();iindex++){
    Serial.print("file: ");
    Serial.println(String(media["media"][0]["fs"][iindex]["n"]));
  }
  /*#if defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
    Serial1.print("ID: ");
  Serial1.println(id);
  Serial1.print("Folder: ");
  Serial1.println(folder);
  Serial1.print("Number of pic: ");
  Serial1.println(pic);
    #endif*/
}

void loop()
{
  if(myTransfer.available())
  {
    Serial.println("incoming...");
    myTransfer.rxObj(message);
    Serial.println(message.command);
    if(message.command == 'f'){
      // code
      Serial.print("file: ");
      Serial.println(String(message.strValue));
      gp.getFile(String(message.strValue));
    } else if(message.command == 't'){
      // code
      Serial.print("thumnail: ");
      Serial.println(String(message.strValue));
      gp.getThumnail(String(message.strValue));
    } else if(message.command == 'c'){
      // code
      Serial.print("connecting go pro");
      Serial.println(String(message.strValue));
      gp.begin();
    }
    
    
  }
  char in = 0;
  
  if (Serial.available() > 0)
  {
    in = Serial.read();
  }
  /*#if defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
    if (Serial1.available() > 0)
  {
    in = Serial1.read();
  }
    #endif*/

  switch (in)
  {
  default:
    break;

  // Connect
  case 'b':
    gp.begin();
    break;

  case 'c':
    Serial.print("Connected: ");
    Serial.println(gp.isConnected() == true ? "Yes" : "No");
    #if defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
    Serial1.print("Connected: ");
    Serial1.println(gp.isConnected() == true ? "Yes" : "No");
    #endif
    break;

  case 'p':
    gp.confirmPairing();
    break;

  case 's':
    Serial.println("Status");
    compute_status();
    break;


  case 'm': // DO NOT USE WHEN CAMERA IS OFF, IT FREEZE ESP
    Serial.println("Media list");
    compute_mediaList();
    break;

  // Turn on and off
  case 'T':
    gp.turnOn();
    break;

  case 't':
    gp.turnOff();
    break;

  // Take a picture of start a video
  case 'A':
    gp.shoot();
    break;

  // Stop the video
  case 'S':
    gp.stopShoot();
    break;

  // Check if it is recording
  case 'r':
    Serial.print("Recording: ");
    #if defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
    Serial1.print("Recording: ");
    Serial1.println(gp.isRecording() == true ? "Yes" : "No");
    #endif
    Serial.println(gp.isRecording() == true ? "Yes" : "No");
    break;

  // Set modes
  case 'V':
    gp.setMode(VIDEO_MODE);
    break;

  case 'P':
    gp.setMode(PHOTO_MODE);
    break;

  case 'M':
    gp.setMode(MULTISHOT_MODE);
    break;

  // Change the orientation
  case 'U':
    gp.setOrientation(ORIENTATION_UP);
    break;

  case 'D':
    gp.setOrientation(ORIENTATION_DOWN);
    break;

  // Change other parameters
  case 'f':
    gp.setVideoFov(MEDIUM_FOV);
    break;

  case 'F':
    gp.setFrameRate(FR_120);
    break;

  case 'R':
    gp.setVideoResolution(VR_1080p);
    break;

  case 'h':
    gp.setPhotoResolution(PR_12MP_WIDE);
    break;

  case 'L':
    gp.setTimeLapseInterval(60);
    break;

  // Localize the camera
  case 'O':
    gp.localizationOn();
    break;

  case 'o':
    gp.localizationOff();
    break;

  // Delete some files, be carefull!
  case 'l':
    gp.deleteLast();
    break;

  case 'g':
    gp.deleteAll();
    break;

  // Print useful data
  case 'd':
    gp.printStatus();
    break;

 case 'n':
 #if defined(ARDUINO_ARCH_ESP32)
 digitalWrite(27,LOW);
 #elif defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)
 digitalWrite(2,LOW);
 #endif
    gp.getThumnail("GOPR4256.JPG");
    break;

  // Close the connection
  case 'X':
    gp.end();
    WiFi.disconnect(true);
    break;
  }
  gp.keepAlive();
}

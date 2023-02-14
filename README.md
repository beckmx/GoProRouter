# GoPro Router
## _Interconnect your gopro camera to your wifi network_

This project requires 2 microcontrollers, those can be:
- 2 ESP32 or 
- 2 Raspberry Pico W(rp2040) or
- 1 ESP32 and 1 Raspberry Pico W(rp2040) 

## How to run it
The project was run using the third option from above, 1 Raspberry pico w and 1 esp32. 
There are 2 arduino projects:
- goproDirect.ino should be programmed in the microcontroller that connects to the gopro (recommended here to use the rp2040 w)
- goproRouter.ino should be programmed to the microcontroller that connects to your wifi router, this can be any esp32.
- Wire both microcontrollers using the Serial pins on the goproDirect and Serial1 on the goproRouter, should be only 4 wires. TX,RX,GND and one extra wire for Hardware up/down.
- Change the WIFI SSID and password on both arduino projects.
- This project contains a folder that should be either placed or replaced in the libraries folder, the original GoProControl is a library published in this repo: [https://github.com/aster94/GoProControl][PlDb], here in this project there are some additional changes that implement transfer messages using this library [https://github.com/PowerBroker2/SerialTransfer][PlDb]

## How does it work
This project interconnects one microcontroller to another one, one of them is connected to the gopro Wifi network and the other connected to your home network.
The communication between them is made with UART ports opened on both sides, right now the baud rate configured is 115200, I havent tested the maximum speed but the thumnails download at a very decent speed. The full images when I tried are downloaded very slow, and is almost useless at 115200 but if you dont mind the speed, that is working.

## Known issues
As said previously, you need 2 microcontrollers. There is no way you can use only 1.
I realized and discovered that the ESP32 link to the gopro Wifi network suffers a lot and at some point breaks and fails to connect afterwards, it is not very useful. Instead I replaced it with a Raspberry Pico W (rp2040) which connects to the GoPro and then it communicates with an esp32 that connects to my Wifi network, this is the most robust scenario I could achieved.
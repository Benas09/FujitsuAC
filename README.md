# FujitsuAC

Fujitsu AC Wifi controller.

This library reverse-engineers parts of the communication used by Fujitsu air conditioners, including those using the FGLair® mobile app.

FGLair is a registered trademark of Fujitsu General Limited. This project is not affiliated with or endorsed by Fujitsu. 
FGLair® app is not required when using this integration - **everything runs local here**.

Using this library you can control your Fujitsu AC through:
* 4 pin connector dedicated to UTY-TFSXW1:
  * Tested with:
  * ASYG09KMTB
  * ASYG09KMCC
  * ASYG12KGTB
  * ASYG09KGTB
  
* USB-type connector dedicated to UTY-TFSXF3:
  * Tested with:
  * ASTG18KMTC
  * ASTG09KMTC

# Support

If you want to support this project, you can buy **ready-to-use** dongle.
Contact: benas.rag@gmail.com

![](/images/dongle1.jpg)
![](/images/dongle2.jpg)

Or you can build it yourself - you will find instructions below :)

# Screenshots
#### HomeAssistant Integration (MQTT Autodiscovery)
![](/images/ha1.png)
![](/images/ha2.png)
![](/images/ha3.png)

#### Credentials page
*This page is available at 192.168.1.1 when connected to the access point created by the dongle, when no config saved yet, or impossible to connect to WiFi*<br/>
![](/images/web.png)

# Building the module

### Required parts
* DC/DC converter 12 -> 5 V (https://www.aliexpress.com/item/1005008257960729.html)
* ESP32 30 pin (https://www.aliexpress.com/item/1005008261897277.html)
* Connector (4P, 10cm/20cm) (https://www.aliexpress.com/item/1005006294406922.html)
* Board (4x6, needs to be trimmed a little bit) (https://www.aliexpress.com/item/1005007024264426.html)
* Logic level converter (https://www.aliexpress.com/item/1005006968679749.html)

  You can also crimp your own plug (Connector PAP-04V-S, pins to crimp: SPHD-002T-P0.5) <- very time consuming if you do not have right tools for it.

### Connection

**!!! IMPORTANT !!!**

**Do not connect anything from air conditioner to external device, like your computer. If you touch AC GND with, lets say laptop GND, it will fry your laptop USB port and/or AC mainboard fuse.**
AC pins are not galvanically isolated and these voltages are not relative to earth GND.

#### JST-type wiring
```
 AC Socket       CN3903                 ESP32
1 (+12V) ------> V in+ ---> V out+ ---> 5V
2 (GND)  ------> V in- ---> V out- ---> GND
3 (DATA) -----------------------------> RX/16 (AC -> ESP)
4 (DATA) -----------------------------> TX/17 (AC <- ESP)
```

Pins from left to right 1 2 3 4 <br/>
![](/images/socket.jpg)

Circuit for JST type connector <br/>
![](/images/circuit.png)

#### USB-type wiring
```
Pinout for USB style socket:
Pin 1 - 12v
Pin 2 - AC_TX - 16 pin
Pin 3 - AC_RX - 17 pin
Pin 4 - GND
```

USB Pinout from top to bottom <br/>
![](/images/usb_plug.png)
<br/>

#### Uploading the code
1. Download Arduino IDE (I used 2.3)
2. File -> Preferences -> Additional boards manager URLs: http://arduino.esp8266.com/stable/package_esp8266com_index.json
3. Download required libraries:
   * this library - FujitsuAC (Benas09)
   * PubSubClient 2.8 (Nick O'Leary)
4. Open Arduino IDE -> File -> Examples -> FujitsuAC -> Controller
5. Select your ESP32 board and upload code (when uploading from MAC, you have to set upload speed to lowest - 460800, otherwise you will get an error while uploading)

*Additional button can be used for credentials reset functionality - uncomment RESET_BUTTON and set to corresponding pin. When you press this button (pull corresponding pin to GND) - controller deletes given credentials, reboots and goes to point*

#### Configuring credentials
1. When controller boots up, it will create access point *Fujitsu-uniqueId*
2. Connect to this access point with your computer/mobile phone
3. Go to 192.168.1.1
4. Fill in WiFi, MQTT credentials, name your device and click Submit. (Device password will be required for OTA updates)
5. Dongle will reboot and connect to your wifi network.
6. If everything is ok, new AC device should appear in HomeAssistant MQTT integration

# DIY Module (Logic level shifter is not included here yet)
![](/images/board_front.jpg)
![](/images/board_back.jpg)
![](/images/board_case.jpg)
![](/images/installed.jpg)

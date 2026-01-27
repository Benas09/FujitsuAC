# FujitsuAC

If you want to support this project, you can buy ready to use dongle. Write me an email: benas.rag@gmail.com

![](/images/dongle1.jpg)
![](/images/dongle2.jpg)

Or you can build it yourself :)

Fujitsu AC Wifi controller.

This library reverse-engineers parts of the communication used by Fujitsu air conditioners, including those using the FGLair® mobile app.

FGLair is a registered trademark of Fujitsu General Limited. This project is not affiliated with or endorsed by Fujitsu. 
FGLair® app is not required when using this integration - **everything runs local here**.

Using this library you can control your Fujitsu AC through 4 pin socket dedicated to UTY-TFSXW1 wifi module.

Tested with:
* ASYG09KMTB
* ASYG09KMCC
* ASYG12KGTB
* ASTG18KMTC

![](/images/homeassistant.png)

## Connection

**!!! IMPORTANT !!!**

**Do not connect any of these pins to external device, like your computer. If you touch AC GND with, lets say laptop GND, it will fry your laptop USB port and/or AC mainboard fuse.**

AC pins are not galvanically isolated and these voltages are not relative to earth GND.

```
 AC Socket       CN3903                 ESP32
1 (+12V) ------> V in+ ---> V out+ ---> 5V
2 (GND)  ------> V in- ---> V out- ---> GND
3 (DATA) -----------------------------> RX 16 (Data to ESP)
4 (DATA) -----------------------------> TX 17 (Data from ESP)
```

Pins from left to right 1 2 3 4 <br/>
![](/images/socket.jpg)

Circuit for JST type connector <br/>
![](/images/circuit.png)

USB Pinout from top to bottom <br/>
![](/images/usb_plug.png)
<br/>
```
Pinout for USB style socket:
Pin 1 - 12v
Pin 2 - AC_TX - 16 pin
Pin 3 - AC_RX - 17 pin
Pin 4 - GND
```

![](/images/board_front.jpg)
![](/images/board_back.jpg)
![](/images/board_case.jpg)
![](/images/installed.jpg)
![](/images/web.png)

### Setup

1. Download Arduino IDE (I used 2.3)
2. Download this library to your "*Arduino IDE*/libraries" (Or directly from Arduino IDE Library manager (search FujitsuAC))
3. Open Arduino IDE -> File -> Examples -> FujitsuAC -> Controller
4. Select your ESP32 board and upload code
5. After controller boots, it will create access point *Fujitsu-uniqueId*
6. Connect to this access point with your computer
7. Go to 192.168.1.1
8. Fill network, MQTT credentials, name your device and click Submit. (Device password will be used for OTA updates)
9. Dongle will reboot and connect to your wifi network.
10. If everything is ok, new AC device should appear in HomeAssistant MQTT integration

Additional button can be used for credentials reset functionality - uncomment RESET_BUTTON and set to corresponding pin. When you press this button (pull corresponding pin to GND) - controller deletes given credentials, reboots and goes to point 5.

To prevent getting electric shock, turn off AC from mains first, then plug in wifi module to dedicated socket and turn on AC again.

If your AC model is not exactly the same as mine, at first I recommend ensure, that registry addresses are the same (change some AC modes with remote control and confirm, that AC state is shown correctly in HomeAssistant)

### Parts list
* DC/DC converter 12 -> 5 V (https://www.aliexpress.com/item/1005008257960729.html)
* ESP32 30 pin (https://www.aliexpress.com/item/1005008261897277.html)
* Connector (4P, 10cm) (https://www.aliexpress.com/item/1005006294406922.html)
* Board (4x6, needs to be trimmed a little bit) (https://www.aliexpress.com/item/1005007024264426.html)
* Logic level converter (https://www.aliexpress.com/item/1005006968679749.html)

  You can also crimp your own plug (Connector PAP-04V-S, pins to crimp: SPHD-002T-P0.5)

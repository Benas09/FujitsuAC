# FujitsuAC

Fujitsu AC Wifi controller.

This library reverse-engineers parts of the communication used by Fujitsu air conditioners, including those using the FGLair® mobile app.

FGLair is a registered trademark of Fujitsu General Limited. This project is not affiliated with or endorsed by Fujitsu. 
FGLair® app is not required when using this integration - **everything runs local here**.

Using this library you can control your Fujitsu AC through 4 pin socket dedicated to UTY-TFSXW1 wifi module.

Works with:
* ASYG09KMTB / AOYG09KMTA

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

Pins from left to right 1 2 3 4
![](/images/socket.jpg)
![](/images/board_front.jpg)
![](/images/board_back.jpg)
![](/images/board_case.jpg)
![](/images/installed.jpg)

### Setup

1. Download Arduino IDE (I used 2.3)
2. Download this library to your "*Arduino IDE*/libraries"
3. Open Arduino IDE -> File -> Examples -> FujitsuAC -> Controller
4. Replace config vars with yours:
   * #define WIFI_SSID "your-ssid"
   * #define WIFI_PASSWORD "your-pw"
   * #define DEVICE_NAME "OfficeAC"
   * #define OTA_PASSWORD "office_ac"
   * #define MQTT_SERVER "192.168.1.100"
   * #define MQTT_PORT 1883
5. Select your ESP32 board and upload code
6. If everything is ok, new AC device should appear in HomeAssistant MQTT integration

To prevent getting electric shock, turn off AC from mains first, then plug in wifi module to dedicated socket and turn on AC again.

If your AC model is not exactly the same as mine, at first I recommend ensure, that registry addresses are the same (change some AC modes with remote control and confirm, that AC state is shown correctly in HomeAssistant)

### Parts list
* DC/DC converter 12 -> 5 V (https://www.aliexpress.com/item/1005008257960729.html)
* ESP32 30 pin (https://www.aliexpress.com/item/1005008261897277.html)
* Connector (4P, 10cm) (https://www.aliexpress.com/item/1005006294406922.html)
* Board (4x6, needs to be trimmed a little bit) (https://www.aliexpress.com/item/1005007024264426.html)

  You can also crimp your own plug (Connector PAP-04V-S, pins to crimp: SPHD-002T-P0.5)

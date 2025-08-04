# FujitsuAC

Fujitsu AC Wifi controller. 
FGLair app is not required, everything is local.

Using this library you can control your Fujitsu AC through 4 pin socket dedicated to UTY-TFSXW1 wifi module.

Works with:
* ASYG09KMTB / AOYG09KMTA

![](/images/homeassistant.png)

## Connection

**!!! IMPORTANT !!!**

**Do not connect any of these pins to external device, like your computer. If you touch AC GND with, lets say laptop GND, it will fry your laptop USB port and/or AC mainboard fuse.**

AC pins are not galvanically isolated and these voltages are not relative to earth GND.

```
 AC Socket       LM2596/CN3903          ESP32
1 (+12V) ------> V in+ ---> V out+ ---> 5V
2 (GND)  ------> V in- ---> V out- ---> GND
3 (DATA) -----------------------------> RX 16 (Data to ESP)
4 (DATA) -----------------------------> TX 17 (Data from ESP)
```

Pins from left to right 1 2 3 4
![](/images/socket.jpg)

* Connector PAP-04V-S
* Pins to crimp: SPHD-002T-P0.5

![](/images/board_front.jpg)
![](/images/board_back.jpg)
![](/images/board_case.jpg)
![](/images/installed.jpg)

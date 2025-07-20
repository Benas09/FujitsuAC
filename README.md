# FujitsuAC

Fujitsu AC Wifi controller

Using this library you can control your Fujitsu AC through 4 pin socket dedicated to UTY-TFSXW1 wifi module.

Works with:
* ASYG09KMTB / AOYG09KMTA

## Connection

**!!! IMPORTANT !!!**

**Do not connect any of these pins to external device, like your computer. If you touch AC GND with, lets say laptop GND, it will fry your laptop USB port and/or AC mainboard fuse.**

AC pins are not galvanically isolated and these voltages are not relative to earth GND.

```
 AC Socket       LM2596                 ESP32
1 (+12V) ------> V in+ ---> V out+ ---> 5V
2 (GND)  ------> V in- ---> V out- ---> GND
3 (DATA) -----------------------------> TX 17
4 (DATA) -----------------------------> RX 16
```

* Connector PAP-04V-S
* Pins to crimp: SPHD-002T-P0.5

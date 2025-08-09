# wiimo

A little application based on the [wiiuse](https://github.com/wiiuse/wiiuse) library and [openFrameworks](http://openframeworks.cc/) that sends [Wiimote](https://wiibrew.org/wiki/Wiimote) data over [OSC](https://en.wikipedia.org/wiki/Open_Sound_Control).

In essence, this is just a simple OF/OSC wrapper for `wiiuse`'s [`example.c`](https://github.com/wiiuse/wiiuse/blob/master/example/example.c).

## Setting up

### wiiuse

Checkout and compile the `wiiuse` library. 
Note that, under Windows, `wiiuse` builds with cmake(-gui) + Visual Studio 2022 out of the box. 
However, it's recommended to the BUILD_SHARED_LIBS definition to the CMake generation, to ensure a DLL is built (instead of a static lib), otherwise linking in Visual Studio will fail (or at least, it will when using a project generated with ProJucer).

### Pairing your Wiimote via Bluetooth

[From the wiki](https://wiibrew.org/wiki/Wiimote#Bluetooth_Pairing):
> If connecting by holding down the 1+2 buttons, the PIN is the bluetooth address of the wiimote backwards, if connecting by pressing the "sync" button on the back of the wiimote, then the PIN is the bluetooth address of the host backwards.

Thus,
```
for an address "00:1E:35:3B:7E:6D":
char pin[6];
pin[0] = 0x6D; pin[1] = 0x7E; pin[2] = 0x3B; 
pin[3] = 0x35; pin[4] = 0x1E; pin[5] = 0x00;
```
You can use this utility to convert a MAC address into a PIN code string for pairing e.g. in Windows: https://mkwpaul.github.io/wiimotePinConverter/

Note:

>For pairing in Windows, the generated PIN code needs to be a valid UTF-8 string - so a 0x00 byte in the MAC address screws things up. You can try to change the MAC with [this](https://macaddresschanger.com/) or [this](https://github.com/thxomas/bdaddr/tree/main), but they seem iffy and don't work for all adapters.


## Usage

Start `wiimo`; your paired wiimotes will be detected. OSC messages will be atomatically sent to the configured host/port, e.g.:

```
ADDRESS(/wiimo/1/mote/rpy) FLOAT(-2.2906117) FLOAT(1.7957518) FLOAT(0)
ADDRESS(/wiimo/1/button/5) BOOL(TRUE)
```
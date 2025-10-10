# USB-CAN Analyzer Linux Support

This is a small C program that dumps the CAN traffic for one these adapters:
![alt text](USB-CAN-A-adapter.jpg)

Windows binary file [stored directly on GitHub](https://github.com/SeeedDocument/USB-CAN_Analyzer).

When plugged in, it will show something like this:
```
Bus 002 Device 006: ID 1a86:7523 QinHeng Electronics HL-340 USB-Serial adapter
```
And the whole thing is actually a USB to serial converter, for which Linux will provide the 'ch341-uart' driver and create a new /dev/ttyUSB device. So this program simply implements part of that serial protocol.

## Build

`canusb.c` can be compile just by running `make` or with:

```
$ gcc canusb.c -o canusb
```
## Usage

```
$ ./canusb -h
Usage: ./canusb <options>
Options:
  -h          Display this help and exit.
  -t          Print TTY/serial traffic debugging info on stderr.
  -d DEVICE   Use TTY DEVICE.
  -s SPEED    Set CAN SPEED in bps.
  -b BAUDRATE Set TTY/serial BAUDRATE (default: 2000000).
  -i ID       Inject using ID (specified as hex string).
  -j DATA     CAN DATA to inject (specified as hex string).
  -n COUNT    Terminate after COUNT frames (default: infinite).
  -g MS       Inject sleep gap in MS milliseconds (default: 200 ms).
  -m MODE     Inject payload MODE (0 = random, 1 = incremental, 2 = fixed).
```


## Example commands:
```
# dump CAN bus traffic from 1 Mbit CAN bus
$ ./canusb -t -d /dev/ttyUSB0 -s 1000000 -t

# send the bytes 0xBEEE from ID 005 on at 1 Mbit CAN bus
$ ./canusb -d /dev/ttyUSB1 -s 1000000 -t -i 5 -j BEEE
```

## Note on USB permissions

You may need change `/dev/ttyUSB*` permissions to access the USB device without root permissions.
To do so you have two options:
1. Add the user to the `dialout` group: `sudo usermod -a -G dialout $USER` and then log out and log back in.
  - `newgrp dialout` should be enough even without logging out/in.
2. Directly change the permissions of the device: `sudo chmod 666 /dev/ttyUSB0` (or whichever device you are using).
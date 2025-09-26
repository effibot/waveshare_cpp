# Waveshare USB-CAN-A C++ Library
This is a C++ library for interfacing with the Waveshare USB-CAN-A device. The library provides functions to initialize the device, send and receive CAN messages, and configure various settings.
## Features
- Initialize and configure the USB-CAN-A device
- Send and receive CAN messages
- Set baud rate and other parameters
- Error handling and status reporting
## Requirements
- C++17 or later
- libusb-1.0


## Implementation Details
The library uses CRTP (Curiously Recurring Template Pattern) to allow for flexible and efficient code reuse. 
- The `base_frame` header defines a common interface for the custom Waveshare protocol frames, so that the end user does not need to deal with low-level details.
- The real frame structures are defined in the `fixed_frame` and `variable_frame` headers, which inherit from `base_frame`.
- The configuration frame is defined in the `config_frame` header, which inherits from `fixed_frame`, as it has a fixed size.
- The `usb_can` header contains the main class for interfacing with the USB-CAN-A device, which uses the frame structures to send and receive data. It will open and close the USB connection, and provide high-level functions for the user.
- The `common` header contains common definitions and utilities used throughout the library, such as constants for the protocol, error codes, and helper functions.

## Protocol Details
The Waveshare USB-CAN-A device uses a custom protocol for communication. The protocol consists of 3 types of frames where each field is represented by a byte unless otherwise specified. The protocol allows to use both CAN 2.0A and CAN 2.0B frames, supporting ID lengths of 11 and 29 bits respectively, depending on the value of the `FRAME_TYPE` field. The constants for the protocol are defined in `common.hpp`, as well with the error codes used in the library.
The most important values are the following:
- `START`: `0xAA`, indicates the start of a frame.
- `HEADER`: `0x55`, indicates the header of a frame (used only in fixed and config frames).
- `TYPE`: `0x01`,`0x02`,`0x12` and a custom value for the variable frame[1]. Indicates the type of the frame, respectivly for fixed data frame, config frame for fixed data frame, config frame for variable data frame and variable data frame.
- `FRAME_TYPE`: `0x01`, `0x02`, `0`, `1`. Indicates if standard or extended CAN ID is used, respectively for fixed and variable frames.
- `FRAME_FORMAT`: `0x01`, `0x02`, `0`, `1`. Indicates if the frame is a data frame or a remote frame, respectively for fixed and variable frames.
- `CANBaud`: `0x01` to `0x0C`. Indicates the baud rate for CAN communication, with values ranging from 1 Mbps to 5 kbps.
- `SerialBaud`: `9600` to `2000000`. Indicates the baud rate for serial communication, with values ranging from 9600 bps to 2 Mbps. They are common values used in serial communication.
- `CANMode`: `0x00`, `0x01`, `0x02`, `0x03`. Indicates the mode of operation for the CAN controller, respectively normal mode, loopback mode, silent mode (listen-only) and silent loopback mode.
- `RTX`: `0x00`, `0x01`. Indicates if the CAN controller should automatically retransmit messages that fail to be acknowledged (`0x00`) or not (`0x01`).
- `RESERVED`: Reserved bytes for future use, should be set to `0x00`.
- `END`: `0x55`, indicates the end of a frame (used only in variable frames).

The frames are as follows:
- **Fixed Frames**: These frames have a fixed size of 20 bytes and are used both for set the configuration of the USB device and for sending/receiving CAN messages with up to 8 bytes of data. The structure of a fixed **data** frame is as follows:
```
[START][HEADER][TYPE][FRAME_TYPE][FRAME_FMT][ID0-ID3][DLC][DATA0-DATA7][RESERVED][CHECKSUM]
```
The structure of a fixed **config** frame is as follows:
```
[START][HEADER][TYPE][CAN_BAUD][FRAME_TYPE][FILTER_ID0-3][MASK_ID0-3][CAN_MODE][AUTO_RTX][RESERVED0-3][CHECKSUM]
```

> For both types of fixed frames, the checksum is calculated as the sum of all bytes from `TYPE` to the last `RESERVED` byte, taking only the least significant byte of the result.


- **Variable Frames**: These frames have a variable size and are used for sending/receiving CAN messages without the need to pad the data to 8 bytes. The structure of the frame is as follows:
```
[START][TYPE][ID][DATA][END]
```
In this case, the `TYPE` field is defined as it's stated in note [1] below and the `ID` field can be either of 2 or 4 bytes, depending on the value of bit 5 of the `TYPE` field. The `DATA` field can be from 0 to 8 bytes, depending on the value of bits 3-0 of the `TYPE` field. The `END` field is always `0x55`.


[1]: The variable frame type is not explicitly defined as a constant but is defined as a byte with the following bit structure:
```
bit 7-6: 11 (indicates variable frame)
bit 5: FRAME_TYPE (0 for standard, 1 for extended)
bit 4: FRAME_FORMAT (0 for data frame, 1 for remote frame)
bit 3-0: DLC (data length code, indicates the number of data bytes, from 0 to 8)
```
> Using a `uint8_t` to represent the variable frame type allows to easily extract the individual fields using bitwise operations. For example, starting with `0xC0` to sets the two most significant bits to `11`, one can do the following:
```cpp
uint8_t type = 0xC0 | (frame_type << 5) | (frame_format << 4) | (dlc & 0x0F);
```

## Note on DLC and Data Padding
The DLC (Data Length Code) field indicates the number of data bytes in the CAN message. It tells how many bytes of `DATA` contains a valid information that should be processed. 
- For fixed frames, the `DATA` field is statically allocated to 8 bytes, so if the DLC is less than 8, the remaining bytes should be padded with zeros. This means that the `DLC` defines how many bytes of the `DATA` field are actually used, while the size of the `DATA` field itself is always 8 bytes.
- For variable frames, the `DATA` field is the one that defines the size of the frame itself, so it's the length of the `DATA` field which defines the DLC.

> This is practically relevant only when you are sending and receiving CAN messages with the USB-CAN-A adapter. When sending a CAN message, another node of the CAN newtork will only see the effective CAN message, not the packet that is used in this library. When receiving a CAN message, the library will parse the serial data and build the appropriate frame structure, so the user will only see the effective CAN message.


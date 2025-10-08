# Waveshare USB-CAN-A C++ Library
This is a C++ library for interfacing with the Waveshare USB-CAN-A device. The library provides functions to initialize the device, send and receive CAN messages using CANOpen standards, and configure USB adapter settings.
## Features
- Initialize and configure the USB-CAN-A device
- Send and receive CAN messages with both fixed and variable length frames
- Support for both standard (11-bit - CAN 2.0A) and extended (29-bit - CAN 2.0B) CAN IDs
- Error handling and status reporting
## Requirements
- C++17 or later
- libusb-1.0


## Implementation Details

- The `FRAME_TYPE` field used in the [protocol User Manual](doc/Waveshare_USB-CAN-A_Users_Manual_EN.pdf) is represented in the library as an enum class `CAN_VERS`, to avoid confusion with the `Type` field which is represented as an enum class `Type` to differentiate between fixed, variable and config frames.

## Protocol Details
The Waveshare USB-CAN-A device uses a custom protocol for communication. The protocol consists of 3 types of frames where each field is represented by a byte unless otherwise specified. The protocol allows to use both CAN 2.0A and CAN 2.0B frames, supporting ID lengths of 11 and 29 bits respectively, depending on the value of the `CAN_VERS` field. 
> The constants for the protocol are defined in `/include/enums/protocol.hpp`.

The most important values are the following:
- `START`: `0xAA`, indicates the start of a frame.
    > indicated as `S` (Start Of Transmission) in the packet diagrams.
- `HEADER`: `0x55`, indicates the header of a frame (used only in fixed and config frames).
    > indicated as `H` (Header) in the packet diagrams.
- `TYPE`: `0x01`,`0x02`,`0x12` and a custom value for the variable frame (see below). Indicates the type of the frame, respectivly for fixed data frame, config frame for fixed data frame, config frame for variable data frame and variable data frame.
    > indicated as `T` (Type) in the packet diagrams.
- `CANVersion`: `0x01`, `0x02`, `0`, `1`. Indicates if standard or extended CAN ID is used, respectively for fixed and variable frames. 
    > indicated as `E` (IsExtended) in the packet diagrams.
- `FRAME_FORMAT`: `0x01`, `0x02`, `0`, `1`. Indicates if the frame is a data frame or a remote frame, respectively for fixed and variable frames.
    > indicated as `F` (Format) in the packet diagrams.
- `CANBaud`: `0x01` to `0x0C`. Indicates the baud rate for CAN communication, with values ranging from 1 Mbps to 5 kbps.
    > indicated as `B` (Baud Rate) in the packet diagrams.
- `SerialBaud`: `9600` to `2000000`. Indicates the baud rate for serial communication, with values ranging from 9600 bps to 2 Mbps. They are common values used in serial communication.    
- `CANMode`: `0x00`, `0x01`, `0x02`, `0x03`. Indicates the mode of operation for the CAN controller, respectively normal mode, loopback mode, silent mode (listen-only) and silent loopback mode.
    > indicated as `M` (Operating Mode) in the packet diagrams.
- `RTX`: `0x00`, `0x01`. Indicates if the CAN controller should automatically retransmit messages that fail to be acknowledged (`0x00`) or not (`0x01`).
    > indicated as `RTX` (Retransmission) in the packet diagrams.
- `RESERVED`: Reserved bytes for future use, should be set to `0x00`.
    > indicated as `R` (Reserved) in the packet diagrams.
- `END`: `0x55`, indicates the end of a frame (used only in variable frames).
    > indicated as `END` (End Of Transmission) in the packet diagrams.

# Frames Structure:

- **Fixed Frames**: These frames have a fixed size of 20 bytes and are used both for set the configuration of the USB device and for sending/receiving CAN messages with up to 8 bytes of data. The structure of a fixed **data** frame is as follows:
![](doc//diagrams/waveshare_fixed_data_packet.md)


- The structure of a fixed **config** frame is as follows:
![](doc//diagrams/waveshare_config_packet.md)

> For both types of fixed frames, the checksum is calculated as the sum of all bytes from `TYPE` to the last `RESERVED` byte, taking only the least significant byte of the result.


- **Variable Frames**: These frames have a variable size and are used for sending/receiving CAN messages without the need to pad the data to 8 bytes. 
    - The `ID` field can be either of 2 or 4 bytes, depending on the value of bit 5 of the `TYPE` field. 
    - The `DATA` field can be from 0 to 8 bytes, depending on the value of bits 3-0 of the `TYPE` field. The `END` field is always `0x55`. 

    The structure of the frame is as follows:
![](doc/diagrams/waveshare_variable_data_packet.md)

    In this case, the `TYPE` field is **not** defined as a constant but as a byte with the following bit structure (**This field should be read from right to left**):
![](doc/diagrams/waveshare_variable_frame_type.md)

    Where `Type` is a constant value of `0xC0` (binary `11000000`), the `IsExtended` bit indicates if the ID is 11 or 29 bits long, the `Format` bit indicates if the frame is a data or remote frame, and the `DLC` (Data Length Code) indicates the number of data bytes in the frame (from 0 to 8).

    > Using a `uint8_t` to represent the `Type` byte allows to easily extract the individual fields using bitwise operations.

    For example, starting with `0xC0` to sets the two most significant bits to `11`, one can do the following:
```cpp
uint8_t type = 0xC0 | (frame_type << 5) | (frame_format << 4) | (dlc & 0x0F);
```

## Note on ID construction
The ID used in the protocol message can be either 11 or 29 bits long, depending on the value of the `CAN_VERS` field. The ID is represented as a 32-bit unsigned integer (`uint32_t`) in the library, but only the lower 11 or 29 bits are used.
> The ID is stored in the protocol message in little endian format, meaning that the least significant byte is stored first. For example, a Standard ID of `0x123` would be stored as: `[0x23][0x01]`



## Note on DLC and Data Padding

The DLC (Data Length Code) field indicates the number of data bytes in the CAN message. It tells how many bytes of `DATA` contains a valid information that should be processed. 

- For fixed frames, the `DATA` field is statically allocated to 8 bytes, so if the DLC is less than 8, the remaining bytes should be padded with zeros. This means that the `DLC` defines how many bytes of the `DATA` field are actually used, while the size of the `DATA` field itself is always 8 bytes.
- For variable frames, the `DATA` field is the one that defines the size of the frame itself, so it's the length of the `DATA` field which defines the DLC.

> This is practically relevant only when you are sending and receiving CAN messages with the USB-CAN-A adapter. When sending a CAN message, another node of the CAN newtork will only see the effective CAN message, not the packet that is used in this library. When receiving a CAN message, the library will parse the serial data and build the appropriate frame structure, so the user will only see the effective CAN message.

## Note on Filtering and Masking

When configuring the acceptance filter and mask, it's important to understand how they work together to determine which CAN messages are accepted by the USB-CAN-A device. 
> It's not necessary to set a filter and/or a mask, but if you do, the following rules apply:
    - The sender's frame ID should match the receiver's filter ID.
    - Both filter and mask are hexadecimal values.
    - The lower 11 bits of filter ID and mask ID are valid in a standard frame (range: `0x00000000~0x000007ff`), and the lower 29 bits of filter ID and mask ID in the extended frame are valid (range `0x00000000~0x1fffffff`).


### Bibliography
- [Waveshare USB-CAN-A User Manual](https://www.waveshare.com/wiki/USB-CAN-A#Software_Settings)
- [Waveshare USB-CAN-A Secondary Development](https://www.waveshare.com/wiki/Secondary_Development_Serial_Conversion_Definition_of_CAN_Protocol)
- [Waveshare USB-CAN-A Driver for Linux](https://files.waveshare.com/wiki/USB-CAN-A/Demo/USB-CAN-A.zip)
- [CAN Specification 2.0](https://www.can-cia.org/can-knowledge/can-cc)

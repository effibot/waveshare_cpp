```mermaid
packet-beta
0: "S" %% Start Of Transmission [0xAA]
1: "H" %% Header [0x55]
2: "T" %% Type 
3: "E" %% IsExtended
4: "F" %% Format
5-8: "ID" %% Identifier
9: "DLC" %% Data Length Code
10-17: "DATA" %% Data Bytes
18: "R" %% Reserved
19: "CHK" %% Checksum
```
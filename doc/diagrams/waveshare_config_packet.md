```mermaid
packet-beta
0: "S" %% Start Of Transmission [0xAA]
1: "H" %% Header [0x55]
2: "T" %% Type 
3: "B" %% Baud Rate
4: "E" %% IsExtended
5-8: "FILTER" %% ID Filter
9-12: "MASK" %% ID Mask
13: "M" %% Operating Mode
14: "RTX" %% Retransmission
15-18: "Reserved" %% Reserved
19: "CHK" %% Checksum
```
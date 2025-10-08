```mermaid
packet-beta
0: "S" %% Start Of Transmission [0xAA]
1: "T" %% Type 
2-5: "ID" %% Identifier
6-13: "DATA" %% Data Bytes (0-8 bytes)
14: "END" %% End Of Transmission [0x55]
```
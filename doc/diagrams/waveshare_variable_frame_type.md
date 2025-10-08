```mermaid
packet-beta
0-3: "DLC" %% Data Length Code (0-8 bytes)
4: "F" %% Format
5: "E" %% IsExtended
6-7: "Type" %% Type constant (always 0xC0)
```
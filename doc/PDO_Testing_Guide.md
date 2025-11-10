# PDO Testing Guide

## PDO Communication Test in test_servo_communication

The `test_servo_communication` script now includes comprehensive PDO (Process Data Object) testing for real-time motor control validation.

### What PDO Testing Does

The PDO test validates:

1. **RPDO1 Transmission** (Host → Motor)
   - Sends controlword and target position
   - COB-ID: `0x200 + node_id`
   - Data: Controlword (2 bytes) + Target Position (4 bytes)

2. **SYNC Message**
   - Sends synchronization message to trigger motor PDO responses
   - COB-ID: `0x80`
   - This is the standard CANopen SYNC object

3. **TPDO1 Reception** (Motor → Host)
   - Receives statusword and actual position feedback
   - COB-ID: `0x180 + node_id`
   - Data: Statusword (2 bytes) + Position Actual (4 bytes)

4. **TPDO2 Reception** (Motor → Host)
   - Receives velocity and current feedback
   - COB-ID: `0x280 + node_id`
   - Data: Velocity Actual (4 bytes) + Current Actual (2 bytes)

### Usage

```bash
# Run full test including PDO communication
./build/scripts/test_servo_communication /dev/ttyUSB0 1

# Or with installed script
./test_servo_communication /dev/ttyUSB0 <node_id>
```

### Test Sequence

The script runs tests in this order:

1. **Device Identification Test**
   - Verifies device type, vendor ID, product code
   - Checks error register

2. **DS402 Communication Test**
   - Reads statusword, mode of operation
   - Reads actual position and velocity

3. **PDO Communication Test** ⭐ NEW
   - Sends RPDO1 with example command
   - Sends SYNC message
   - Listens for TPDO1 and TPDO2 responses
   - Displays received feedback data

4. **Basic Control Test**
   - Enable/disable sequence (with confirmation)

### Expected Output (PDO Section)

```
=== PDO Communication Test ===
Testing Process Data Objects (PDOs) for real-time communication
  RPDO1 COB-ID: 0x201 (commands to motor)
  RPDO2 COB-ID: 0x301
  TPDO1 COB-ID: 0x181 (feedback from motor)
  TPDO2 COB-ID: 0x281
  SYNC COB-ID: 0x80

1. Testing RPDO1 (sending controlword + target position)...
  ✓ RPDO1 sent: CW=0x6 Target=1000

2. Testing SYNC-triggered TPDOs...
  Sending SYNC message...
  ✓ SYNC sent (COB-ID 0x80)
  Listening for TPDO responses (5 second timeout)...
  ✓ TPDO1 received:
    Statusword: 0x237
    Position: 12345 counts
    State: Operation enabled
    Bits: RTSO SO OE VE QS 
  ✓ TPDO2 received:
    Velocity: 5000 counts/sec
    Current: 150 mA

  PDO Test Results:
    RPDO1 (send):    ✓ Sent successfully
    SYNC:            ✓ Sent successfully
    TPDO1 (receive): ✓ Received
    TPDO2 (receive): ✓ Received
```

### Troubleshooting

**If TPDOs are not received:**

1. **Check PDO Configuration in Motor**
   - TPDOs may need to be enabled in the object dictionary
   - Check objects 0x1800 (TPDO1 parameters) and 0x1A00 (TPDO1 mapping)
   - Check objects 0x1801 (TPDO2 parameters) and 0x1A01 (TPDO2 mapping)

2. **Verify SYNC Configuration**
   - Motor must be configured to respond to SYNC messages
   - Check transmission type in TPDO communication parameters

3. **Check COB-IDs**
   - Ensure node_id matches your motor configuration
   - Default PDO COB-IDs may be customized in some motors

4. **CAN Bus Configuration**
   - Verify 500kbps baud rate
   - Check termination resistors (120Ω at each end)
   - Verify CAN-H and CAN-L wiring

### PDO Mapping Reference (CIA402 Standard)

#### TPDO1 (Transmit PDO 1 - Motor → Host)
Default mapping for statusword and position feedback:
- Object 0x1A00: TPDO1 Mapping
  - Subindex 0: Number of mapped objects = 2
  - Subindex 1: 0x60410010 (Statusword, 16 bits)
  - Subindex 2: 0x60640020 (Position Actual Value, 32 bits)

#### TPDO2 (Transmit PDO 2 - Motor → Host)
Default mapping for velocity and current:
- Object 0x1A01: TPDO2 Mapping
  - Subindex 0: Number of mapped objects = 2
  - Subindex 1: 0x606C0020 (Velocity Actual Value, 32 bits)
  - Subindex 2: 0x60770010 (Torque/Current Actual Value, 16 bits)

#### RPDO1 (Receive PDO 1 - Host → Motor)
Default mapping for control commands:
- Object 0x1600: RPDO1 Mapping
  - Subindex 0: Number of mapped objects = 2
  - Subindex 1: 0x60400010 (Controlword, 16 bits)
  - Subindex 2: 0x607A0020 (Target Position, 32 bits)

### Integration with ROS2

Once PDO testing passes, you can use the PDO manager in your ROS2 application:

```cpp
#include "canopen/pdo_manager.hpp"

auto socket = create_socket("vcan0");
PDOManager pdo_mgr(socket);

// Register callback for motor feedback
pdo_mgr.register_tpdo1_callback(node_id, [](const can_frame& frame) {
    // Parse statusword and position from frame.data
    uint16_t statusword = frame.data[0] | (frame.data[1] << 8);
    int32_t position = frame.data[2] | (frame.data[3] << 8) | 
                       (frame.data[4] << 16) | (frame.data[5] << 24);
    // Process feedback...
});

pdo_mgr.start();

// Send commands
std::vector<uint8_t> cmd = {/* controlword + target position */};
pdo_mgr.send_rpdo1(node_id, cmd);
```

### Notes

- PDO communication is **real-time** - no request/response overhead like SDO
- Typical cycle time: 1-10ms (vs SDO: 10-50ms)
- PDOs are event-driven or SYNC-triggered
- Maximum 8 bytes per PDO frame
- For cyclic position/velocity control, use RPDO/TPDO pairs with SYNC

# Prompt 5: Testing & Validation

I need comprehensive testing for the WaveshareSocketCANBridge:

## Test Categories

### 1. Unit Tests (Catch2)

- SocketCAN socket setup/teardown
- Frame conversion (VariableFrame ↔ struct can_frame)
- read_variable_frame_bytes() with mock data
- Configuration handshake (ConfigFrame send/ACK)

### 2. Integration Tests

- Full bridge lifecycle (configure → start → stop)
- Bidirectional forwarding (loopback via vcan0)
- Error handling (USB disconnect, SocketCAN write failure)
- Statistics accuracy

### 3. ROS2 Tests

- Lifecycle transitions
- Parameter validation
- SocketCAN message round-trip

## Mock Requirements

- MockUSBAdapter for unit tests (no physical device needed)
- vcan0 loopback for integration tests
- CAN message generators (standard/extended ID, various DLC)

## Deliverables

Please provide:

1. test/test_socketcan_bridge.cpp (Catch2 unit tests)
2. test/integration/test_bridge_lifecycle.cpp
3. Mock adapter implementation if needed
4. ROS2 test launch file (test/bridge_integration.test.py)

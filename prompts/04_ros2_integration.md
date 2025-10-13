# Prompt 4: ROS2 Integration Planning

The WaveshareSocketCANBridge is implemented. Now I need a ROS2 lifecycle node to manage it:

## Requirements

1. Lifecycle node with configure/activate/deactivate/cleanup transitions
2. Parameters: usb_device, can_interface, serial_baud, can_baud
3. on_configure(): Create bridge, call bridge->configure()
4. on_activate(): Call bridge->start(), spawn statistics timer
5. on_deactivate(): Call bridge->stop(), destroy timer
6. Statistics publisher: Periodic (5s) logging of bridge stats

## Architecture

```
ROS2 LifecycleNode (waveshare_bridge_node)
    ↓ owns
WaveshareSocketCANBridge
    ↓ owns
USBAdapter (/dev/ttyCANBUS0)
    ↓ serial I/O
Waveshare USB-CAN-A
    ↓ CAN bus
    
ROS2 LifecycleNode also:
    ↓ creates
SocketCAN interface (vcan0/can0)
    ↑ used by
Other ROS2 nodes (via ros2_socketcan)
```

## Other ROS2 Nodes Will

- Subscribe to SocketCAN for CAN frame ingestion (TPDO processing)
- Publish to SocketCAN for CAN frame transmission (RPDO, SDO requests)
- Use standard can_msgs/msg/Frame message type

## Deliverables

Please provide:

1. Lifecycle node implementation (ros2_ws/src/waveshare_can_bridge/src/bridge_node.cpp)
2. Parameter mapping helper functions (serial_baud int → SerialBaud enum)
3. Launch file with parameter configuration (config/bridge.yaml + bridge.launch.py)
4. CMakeLists.txt and package.xml for the ROS2 package

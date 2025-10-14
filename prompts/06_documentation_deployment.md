# Prompt 6: Documentation & Deployment

Final phase: Documentation and deployment preparation.

## Documentation Needed

### 1. Architecture diagram updates

- Add SocketCAN bridge layer to doc/diagrams/architecture.md
- Sequence diagram: ROS2 node → SocketCAN → Bridge → USB → CAN bus

### 2. API documentation

- Doxygen comments for WaveshareSocketCANBridge public methods
- Usage examples in README.md (configuration, lifecycle management)

### 3. Deployment guide

- Linux setup (vcan0 interface, udev rules for /dev/ttyCANBUS0)
- ROS2 workspace setup (colcon build, sourcing)
- Troubleshooting (common errors, logging, debugging)

## Deployment Artifacts

### 1. Udev rule

File: `/etc/udev/rules.d/99-waveshare-can.rules`

```bash
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="ttyCANBUS%n"
```

### 2. Systemd service (optional)

Auto-start bridge on boot

### 3. Docker support (optional)

Containerized ROS2 + bridge

## Deliverables

Please provide:

1. Updated README.md with SocketCAN bridge section
2. doc/DEPLOYMENT.md with step-by-step Linux setup
3. Mermaid diagram updates (architecture, sequence)
4. Udev rule and systemd service files

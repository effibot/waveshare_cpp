#!/usr/bin/env bash
# Diagnostic script for USB-CAN adapter setup
# Helps identify configuration and wiring issues

echo "====================================="
echo "CAN Adapter Setup Diagnostics"
echo "====================================="
echo ""

# Function to check device
check_device() {
    local dev=$1
    echo -n "Checking $dev... "
    if [ -e "$dev" ]; then
        echo "✓ EXISTS"
        echo "  Permissions: $(ls -l $dev | awk '{print $1, $3, $4}')"
        echo "  Driver: $(udevadm info -q property -n $dev | grep ID_USB_DRIVER | cut -d= -f2)"
        return 0
    else
        echo "✗ NOT FOUND"
        return 1
    fi
}

# Check for USB-CAN devices
echo "1. USB Device Detection:"
echo "------------------------"
check_device "/dev/ttyUSB0"
echo ""
check_device "/dev/ttyUSB1"
echo ""

# Check USB device info
echo "2. USB Device Information:"
echo "------------------------"
echo "Connected USB serial devices:"
lsusb | grep -i "ch340\|ch341\|serial" || echo "  No CH340/CH341 devices found"
echo ""

# Check permissions
echo "3. User Permissions:"
echo "------------------------"
echo "Current user: $(whoami)"
echo "Groups: $(groups)"
echo -n "Can access serial ports: "
if groups | grep -q dialout; then
    echo "✓ YES (in 'dialout' group)"
else
    echo "✗ NO (not in 'dialout' group)"
    echo "  Run: sudo usermod -a -G dialout $(whoami)"
    echo "  Then log out and log back in"
fi
echo ""

# Check for running processes
echo "4. Running CAN Processes:"
echo "------------------------"
pgrep -a wave_ || echo "  No wave_* processes running"
echo ""

# Check SocketCAN interfaces
echo "5. SocketCAN Interfaces:"
echo "------------------------"
ip link show | grep -A2 "can\|vcan" || echo "  No CAN interfaces found"
echo ""

# Build status
echo "6. Build Status:"
echo "------------------------"
if [ -x "./build/scripts/wave_reader" ]; then
    echo "✓ wave_reader built"
else
    echo "✗ wave_reader not found"
fi

if [ -x "./build/scripts/wave_writer" ]; then
    echo "✓ wave_writer built"
else
    echo "✗ wave_writer not found"
fi

if [ -x "./build/scripts/wave_bridge" ]; then
    echo "✓ wave_bridge built"
else
    echo "✗ wave_bridge not found"
fi
echo ""

echo "====================================="
echo "Hardware Checklist:"
echo "====================================="
cat <<EOF
For proper CAN bus operation, verify:

1. Physical Wiring:
   - Both adapters connected to same CAN H and CAN L lines
   - CAN H to CAN H, CAN L to CAN L (not crossed!)
   
2. Termination:
   - 120Ω resistor at each end of bus
   - Total bus resistance ~60Ω (use multimeter between CAN H and CAN L)
   
3. Configuration:
   - Same baud rate on both adapters (default: 1000000)
   - Both in NORMAL mode (not LISTEN_ONLY)
   - Filters set to accept all frames (0x00000000)
   
4. Testing Strategy:
   - Reader on USB0, Writer on USB1 (or vice versa)
   - Can NOT run reader and writer on same device simultaneously
   - Bridge uses one device, external tool uses the other
   
Common Issues:
- Silent receiver: Check termination resistors
- "Device busy": Another process is using the device
- "Permission denied": User not in 'dialout' group
- Checksum errors: Wrong baud rate or electrical noise
EOF

echo ""
echo "====================================="
echo "Next Steps:"
echo "====================================="
echo "1. Fix any issues shown above"
echo "2. Run: ./scripts/test_dual_adapter_setup.sh"
echo "3. If test fails, verify hardware with oscilloscope/logic analyzer"

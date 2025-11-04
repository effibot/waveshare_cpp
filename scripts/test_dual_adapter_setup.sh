#!/usr/bin/env bash
# Test script for dual USB-CAN adapter setup
# This verifies communication between two adapters on the same CAN bus

set -e

BUILD_DIR="./build/scripts"
USB0="/dev/ttyUSB0"
USB1="/dev/ttyUSB1"
SERIAL_BAUD="2000000"
CAN_BAUD="1000000"

echo "====================================="
echo "Dual Adapter CAN Communication Test"
echo "====================================="
echo ""
echo "Setup:"
echo "  USB0: ${USB0} (Reader)"
echo "  USB1: ${USB1} (Writer)"
echo "  Serial Baud: ${SERIAL_BAUD}"
echo "  CAN Baud: ${CAN_BAUD}"
echo ""
echo "Test: Writer on USB1 sends frames, Reader on USB0 receives them"
echo ""

# Check if devices exist
if [ ! -e "$USB0" ]; then
    echo "ERROR: Device $USB0 not found!"
    exit 1
fi

if [ ! -e "$USB1" ]; then
    echo "ERROR: Device $USB1 not found!"
    exit 1
fi

# Check if executables exist
if [ ! -x "$BUILD_DIR/wave_reader" ]; then
    echo "ERROR: wave_reader not found or not executable!"
    echo "Run: cmake --build build"
    exit 1
fi

if [ ! -x "$BUILD_DIR/wave_writer" ]; then
    echo "ERROR: wave_writer not found or not executable!"
    echo "Run: cmake --build build"
    exit 1
fi

echo "Starting reader on $USB0 in background..."
$BUILD_DIR/wave_reader -d "$USB0" -s "$SERIAL_BAUD" -b "$CAN_BAUD" -f variable &
READER_PID=$!

echo "Reader PID: $READER_PID"
echo ""

# Give reader time to start
sleep 2

echo "Sending 5 test frames from $USB1..."
$BUILD_DIR/wave_writer \
    -d "$USB1" \
    -s "$SERIAL_BAUD" \
    -b "$CAN_BAUD" \
    -f variable \
    -i 0x042 \
    -j "694200" \
    -n 5 \
    -g 500

echo ""
echo "Waiting 3 seconds for frames to be received..."
sleep 3

echo ""
echo "Stopping reader..."
kill -SIGINT $READER_PID 2>/dev/null || true
wait $READER_PID 2>/dev/null || true

echo ""
echo "====================================="
echo "Test Complete"
echo "====================================="
echo ""
echo "Expected output:"
echo "  - Reader should have printed 5 'Received <<' lines"
echo "  - Each line should show CAN ID 0x042 with data [69 42 00]"
echo ""
echo "If no frames were received:"
echo "  1. Check CAN bus termination (120Ω resistors)"
echo "  2. Verify both adapters are on the same physical bus"
echo "  3. Check CAN H and CAN L wiring"
echo "  4. Ensure both adapters configured with same baud rate"

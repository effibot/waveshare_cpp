# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = canusb
SOURCE = canusb.c

# Default device and settings
DEFAULT_DEVICE = /dev/ttyUSB0
DEFAULT_SPEED = 1000000
DEFAULT_BAUDRATE = 2000000

# Build target
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

# Clean target
clean:
	rm -f $(TARGET)

# Install target (copy to /usr/local/bin)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall target
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Run targets with default commands
run-dump: $(TARGET)
	@echo "Dumping CAN bus traffic from $(DEFAULT_DEVICE) at $(DEFAULT_SPEED) bps..."
	@echo "Press Ctrl+C to stop"
	./$(TARGET) -t -d $(DEFAULT_DEVICE) -s $(DEFAULT_SPEED)

run-dump-quiet: $(TARGET)
	@echo "Dumping CAN bus traffic (quiet mode) from $(DEFAULT_DEVICE) at $(DEFAULT_SPEED) bps..."
	@echo "Press Ctrl+C to stop"
	./$(TARGET) -d $(DEFAULT_DEVICE) -s $(DEFAULT_SPEED)

run-test-inject: $(TARGET)
	@echo "Sending test frame with ID 005 and data BEEE to $(DEFAULT_DEVICE) at $(DEFAULT_SPEED) bps..."
	./$(TARGET) -d $(DEFAULT_DEVICE) -s $(DEFAULT_SPEED) -t -i 5 -j BEEE -n 5

run-continuous-inject: $(TARGET)
	@echo "Continuously sending frames with ID 123 and incremental data to $(DEFAULT_DEVICE)..."
	@echo "Press Ctrl+C to stop"
	./$(TARGET) -d $(DEFAULT_DEVICE) -s $(DEFAULT_SPEED) -t -i 123 -j DEADBEEF -m 1 -g 500

help:
	@echo "Available targets:"
	@echo "  make            - Build the canusb program"
	@echo "  make clean      - Remove built files"
	@echo "  make install    - Install to /usr/local/bin"
	@echo "  make uninstall  - Remove from /usr/local/bin"
	@echo ""
	@echo "Run targets:"
	@echo "  make run-dump           - Dump CAN traffic with debug output"
	@echo "  make run-dump-quiet     - Dump CAN traffic without debug output"
	@echo "  make run-test-inject    - Send 5 test frames and exit"
	@echo "  make run-continuous-inject - Continuously send frames with incremental data"
	@echo ""
	@echo "Configuration:"
	@echo "  DEFAULT_DEVICE = $(DEFAULT_DEVICE)"
	@echo "  DEFAULT_SPEED = $(DEFAULT_SPEED)"
	@echo "  DEFAULT_BAUDRATE = $(DEFAULT_BAUDRATE)"
	@echo ""
	@echo "Note: You may need to add your user to the dialout group:"
	@echo "  sudo usermod -a -G dialout $$USER"

# Set default target
.DEFAULT_GOAL := $(TARGET)

# Declare phony targets
.PHONY: clean install uninstall run-dump run-dump-quiet run-test-inject run-continuous-inject help
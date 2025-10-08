#!/bin/bash
# build_and_test.sh - Convenient script to build and run tests

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building Waveshare USB-CAN-A Tests...${NC}"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build . --parallel "$(nproc)"

# Run tests
echo -e "${GREEN}Running tests...${NC}"
./test_config_frame "$@"
TEST_RESULT=$?

if [ $TEST_RESULT -eq 0 ]; then
    ./test_fixed_frame "$@"
    TEST_RESULT=$?
fi

if [ $TEST_RESULT -eq 0 ]; then
    ./test_variable_frame "$@"
    TEST_RESULT=$?
fi

# Show summary
if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
else
    echo -e "${RED}❌ Some tests failed!${NC}"
    exit 1
fi

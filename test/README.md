# C++ Testing Guide for Waveshare USB-CAN-A

This directory contains unit tests for the Waveshare USB-CAN-A C++ library using the **Catch2** testing framework.

## 🎯 Testing Best Practices Used

### 1. **Test Organization**
- **Test Fixtures**: Reusable setup code in `ConfigFrameFixture` struct
- **SECTION blocks**: Group related test scenarios within a test case
- **Tags**: Categorize tests for selective execution (e.g., `[config]`, `[baud]`, `[integration]`)

### 2. **AAA Pattern** (Arrange-Act-Assert)
Every test follows this structure:
```cpp
TEST_CASE("Description") {
    // ARRANGE: Set up test conditions
    ConfigFrame frame;
    
    // ACT: Perform the operation
    frame.set_baud_rate(CANBaud::BAUD_500K);
    
    // ASSERT: Verify the result
    REQUIRE(frame.get_baud_rate() == CANBaud::BAUD_500K);
}
```

### 3. **Comprehensive Coverage**
- ✅ Default values and initialization
- ✅ Boundary values (min, max, typical)
- ✅ Edge cases and error conditions
- ✅ State changes and field independence
- ✅ Integration scenarios

### 4. **Self-Documenting Tests**
- Descriptive test names that explain expected behavior
- Comments explaining WHY, not just WHAT
- Tests serve as usage examples for the API

## 📦 Quick Start

### Option 1: Using CMake (Recommended)

```bash
# From the tests directory
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure

# Or run tests directly
./test_config_frame
```

CMake will automatically download and configure Catch2 if not found on your system.

### Option 2: Install Catch2 System-Wide

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install catch2
```

#### Arch Linux:
```bash
sudo pacman -S catch2
```

#### From Source:
```bash
git clone https://github.com/catchorg/Catch2.git
cd Catch2
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build/ --target install
```

### Option 3: Manual Compilation (No CMake)

```bash
# Compile test
g++ -std=c++17 -I../include -I/path/to/catch2/include \
    test_config_frame.cpp \
    ../src/config_frame.cpp \
    ../src/fixed_frame.cpp \
    ../src/variable_frame.cpp \
    -o test_config_frame

# Run tests
./test_config_frame
```

## 🎮 Running Tests

### Run All Tests
```bash
./test_config_frame
```

### Run Tests by Tag
```bash
# Only baud rate tests
./test_config_frame [baud]

# Only integration tests
./test_config_frame [integration]

# All config tests
./test_config_frame [config]

# Exclude certain tags
./test_config_frame ~[slow]
```

### Run Specific Test Cases
```bash
# Run tests matching pattern
./test_config_frame "ConfigFrame - Baud rate*"

# Run with verbose output
./test_config_frame -v

# Show successful tests too
./test_config_frame -s
```

### List Available Tests
```bash
# List all tests
./test_config_frame --list-tests

# List all tags
./test_config_frame --list-tags
```

## 📊 Understanding Test Output

### Success:
```
All tests passed (48 assertions in 13 test cases)
```

### Failure Example:
```
test_config_frame.cpp:75: FAILED:
  REQUIRE( frame.get_baud_rate() == CANBaud::BAUD_500K )
with expansion:
  BAUD_1M == BAUD_500K
```

Catch2 shows you:
- **File and line number** where assertion failed
- **Actual values** of both sides of comparison
- **Expanded expression** showing what was compared

## 🏗️ Test Structure

### Test File Organization
```cpp
// 1. Test Fixture (shared setup)
struct ConfigFrameFixture {
    // Common test data and helper functions
};

// 2. Test Cases with Sections
TEST_CASE_METHOD(ConfigFrameFixture, "Description", "[tags]") {
    SECTION("Scenario 1") { /* ... */ }
    SECTION("Scenario 2") { /* ... */ }
}
```

### Available Assertions
```cpp
// Basic assertions
REQUIRE(condition);              // Must pass
CHECK(condition);                // Can fail, test continues

// Comparison assertions
REQUIRE(a == b);
REQUIRE(a != b);
REQUIRE(a < b);

// Floating point
REQUIRE(value == Approx(3.14).epsilon(0.01));

// String matchers
REQUIRE_THAT(str, Catch::Contains("substring"));
REQUIRE_THAT(str, Catch::StartsWith("prefix"));
REQUIRE_THAT(str, Catch::EndsWith("suffix"));

// Exception testing
REQUIRE_THROWS(function());
REQUIRE_THROWS_AS(function(), std::runtime_error);
REQUIRE_NOTHROW(function());
```

## 📝 Adding New Tests

### Step 1: Add Test Case
```cpp
TEST_CASE_METHOD(ConfigFrameFixture, "Your feature description", "[your-tag]") {
    // Arrange
    ConfigFrame frame;
    
    // Act
    frame.your_method();
    
    // Assert
    REQUIRE(frame.get_something() == expected);
}
```

### Step 2: Rebuild
```bash
cd build
cmake --build .
```

### Step 3: Run
```bash
./test_config_frame
```

## 🔍 Debugging Failed Tests

### 1. Run with Verbose Output
```bash
./test_config_frame -s -v
```

### 2. Run Specific Failing Test
```bash
./test_config_frame "Exact test name"
```

### 3. Use GDB
```bash
gdb ./test_config_frame
(gdb) run
(gdb) backtrace  # When test fails
```

### 4. Add Debug Output
```cpp
TEST_CASE("Debugging") {
    ConfigFrame frame;
    INFO("Baud rate: " << static_cast<int>(frame.get_baud_rate()));
    REQUIRE(condition);  // INFO is printed if this fails
}
```

## 📚 Test Categories (Tags)

| Tag | Description |
|-----|-------------|
| `[config]` | All ConfigFrame tests |
| `[baud]` | Baud rate configuration tests |
| `[mode]` | CAN mode tests |
| `[filter]` | Filter configuration tests |
| `[mask]` | Mask configuration tests |
| `[rtx]` | Auto-retransmission tests |
| `[constructor]` | Constructor tests |
| `[integration]` | End-to-end workflow tests |
| `[checksum]` | Checksum-related tests |
| `[size]` | Frame size tests |

## 🚀 Next Steps

1. **Add More Test Files**: Create `test_fixed_frame.cpp`, `test_variable_frame.cpp`
2. **Add to CMakeLists.txt**: Register new test executables
3. **Continuous Integration**: Set up GitHub Actions to run tests automatically
4. **Coverage Analysis**: Use `gcov`/`lcov` to measure test coverage

## 📖 Additional Resources

- [Catch2 Documentation](https://github.com/catchorg/Catch2/blob/devel/docs/Readme.md)
- [Catch2 Tutorial](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md)
- [C++ Testing Best Practices](https://github.com/cpp-best-practices/cppbestpractices)

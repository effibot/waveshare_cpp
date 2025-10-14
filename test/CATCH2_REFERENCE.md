# Catch2 Quick Reference Guide

## 📝 Most Common Assertions

### Basic Assertions
```cpp
REQUIRE(expression);              // Test fails if false
CHECK(expression);                // Test continues if false
REQUIRE_FALSE(expression);        // Test fails if true
```

### Comparisons
```cpp
REQUIRE(a == b);                  // Equality
REQUIRE(a != b);                  // Inequality  
REQUIRE(a < b);                   // Less than
REQUIRE(a <= b);                  // Less than or equal
REQUIRE(a > b);                   // Greater than
REQUIRE(a >= b);                  // Greater than or equal
```

### String Matchers
```cpp
using Catch::Matchers::Contains;
using Catch::Matchers::StartsWith;
using Catch::Matchers::EndsWith;

REQUIRE_THAT(str, Contains("substring"));
REQUIRE_THAT(str, StartsWith("prefix"));
REQUIRE_THAT(str, EndsWith("suffix"));
REQUIRE_THAT(str, Equals("exact match"));
```

### Floating Point
```cpp
REQUIRE(value == Approx(3.14));           // Default epsilon
REQUIRE(value == Approx(3.14).epsilon(0.01));  // Custom epsilon
REQUIRE(value == Approx(3.14).margin(0.1));    // Absolute margin
```

### Exception Testing
```cpp
REQUIRE_THROWS(function());                    // Any exception
REQUIRE_THROWS_AS(function(), std::exception); // Specific type
REQUIRE_NOTHROW(function());                   // No exception
```

## 🏗️ Test Structure

### Basic Test Case
```cpp
TEST_CASE("Description", "[tag1][tag2]") {
    // Test code
}
```

### Test Case with Fixture
```cpp
struct MyFixture {
    int value = 42;
};

TEST_CASE_METHOD(MyFixture, "Test using fixture", "[fixture]") {
    REQUIRE(value == 42);
}
```

### Sections (BDD Style)
```cpp
TEST_CASE("Container operations", "[container]") {
    std::vector<int> v;
    
    SECTION("when empty") {
        REQUIRE(v.empty());
    }
    
    SECTION("after adding element") {
        v.push_back(1);
        REQUIRE(v.size() == 1);
    }
}
```

### Template Tests
```cpp
TEMPLATE_TEST_CASE("Test with types", "[template]", int, double, float) {
    TestType value = TestType{};
    REQUIRE(value == TestType{});
}
```

## 🎯 Advanced Features

### Info Context (Only Shown on Failure)
```cpp
TEST_CASE("With context") {
    int x = 42;
    INFO("Value of x: " << x);
    REQUIRE(x > 100);  // Fails and shows INFO
}
```

### Scoped Info
```cpp
TEST_CASE("Scoped info") {
    SCOPED_INFO("This info applies to rest of test");
    REQUIRE(condition);
}
```

### Capture Variables
```cpp
TEST_CASE("Capture") {
    int x = 42;
    CAPTURE(x);  // Shows x value on failure
    REQUIRE(x > 100);
}
```

### Generate Test Cases
```cpp
GENERATE(1, 2, 3, 4, 5);
GENERATE(range(1, 10));
GENERATE(values({1.0, 2.5, 3.14}));
```

## 🏃 Running Tests

### Basic Execution
```bash
./test_executable                 # Run all tests
./test_executable -s             # Show successful assertions
./test_executable -v             # Verbose output
```

### Filtering
```bash
./test_executable [tag]          # Run tests with tag
./test_executable ~[tag]         # Exclude tests with tag
./test_executable "Test name"    # Run specific test
./test_executable "Test*"        # Wildcard matching
```

### Listing
```bash
./test_executable --list-tests   # List all tests
./test_executable --list-tags    # List all tags
./test_executable --list-test-names-only  # Just names
```

### Output Control
```bash
./test_executable --success      # Show successful tests
./test_executable --break        # Break into debugger on failure
./test_executable --durations yes # Show test timings
```

## 🎨 Best Practices for Your Project

### 1. Naming Convention
```cpp
TEST_CASE("ClassName - What it does when condition", "[category]")
```

Example:
```cpp
TEST_CASE("ConfigFrame - Sets baud rate when valid value provided", "[config][baud]")
```

### 2. Arrange-Act-Assert Pattern
```cpp
TEST_CASE("Clear test structure") {
    // ARRANGE: Set up test data
    ConfigFrame frame;
    
    // ACT: Perform the operation
    frame.set_baud_rate(CANBaud::BAUD_500K);
    
    // ASSERT: Verify result
    REQUIRE(frame.get_baud_rate() == CANBaud::BAUD_500K);
}
```

### 3. Test One Thing
```cpp
// Good - tests one specific behavior
TEST_CASE("Filter is stored correctly") {
    ConfigFrame frame;
    frame.set_filter(0x123);
    REQUIRE(frame.get_filter() == 0x123);
}

// Avoid - tests multiple things
TEST_CASE("All config works") {  // Too broad
    // ... tests baud, mode, filter, mask, etc.
}
```

### 4. Use Sections for Related Scenarios
```cpp
TEST_CASE("Baud rate configuration", "[baud]") {
    ConfigFrame frame;
    
    SECTION("high speed rates") {
        frame.set_baud_rate(CANBaud::BAUD_1M);
        REQUIRE(frame.get_baud_rate() == CANBaud::BAUD_1M);
    }
    
    SECTION("low speed rates") {
        frame.set_baud_rate(CANBaud::BAUD_10K);
        REQUIRE(frame.get_baud_rate() == CANBaud::BAUD_10K);
    }
}
```

### 5. Test Edge Cases
```cpp
TEST_CASE("Boundary values") {
    ConfigFrame frame;
    
    // Minimum value
    frame.set_filter(0x000);
    REQUIRE(frame.get_filter() == 0x000);
    
    // Maximum standard ID
    frame.set_filter(0x7FF);
    REQUIRE(frame.get_filter() == 0x7FF);
    
    // Maximum extended ID
    frame.set_filter(0x1FFFFFFF);
    REQUIRE(frame.get_filter() == 0x1FFFFFFF);
}
```

## 🔧 Debugging Tips

### 1. Run Single Test with Verbose Output
```bash
./test_executable "Exact test name" -s -v
```

### 2. Add Debug Prints
```cpp
TEST_CASE("Debug") {
    ConfigFrame frame;
    std::cout << "Debug: " << static_cast<int>(frame.get_baud_rate()) << std::endl;
    REQUIRE(condition);
}
```

### 3. Use INFO for Context
```cpp
TEST_CASE("With context") {
    for (int i = 0; i < 10; ++i) {
        INFO("Iteration: " << i);  // Shown only on failure
        REQUIRE(some_check(i));
    }
}
```

### 4. Break on Failure
```bash
./test_executable --break  # Breaks into debugger
```

## 📊 Tags Organization

Suggested tag hierarchy:
- `[unit]` - Unit tests
- `[integration]` - Integration tests  
- `[slow]` - Slower tests
- `[component-name]` - By component (e.g., `[config]`, `[fixed]`, `[variable]`)
- `[feature]` - By feature (e.g., `[checksum]`, `[filter]`, `[baud]`)

Run combinations:
```bash
./test [unit][config]        # Unit tests for config
./test [integration]~[slow]  # Integration tests, not slow
```

## 🎓 Your Project Specific Examples

### Testing Your Result<T> Type
```cpp
TEST_CASE("Result type validation", "[result]") {
    auto result = frame.validate();
    
    // Check success
    REQUIRE(result.ok());
    REQUIRE_FALSE(result.fail());
    
    // Or check error
    REQUIRE(result.fail());
    REQUIRE(result.error() == Status::WBAD_ID);
}
```

### Testing CRTP Interfaces
```cpp
TEMPLATE_TEST_CASE("DataInterface methods", "[interface]", 
                   FixedFrame, VariableFrame) {
    TestType frame;
    frame.set_id(0x123);
    REQUIRE(frame.get_id() == 0x123);
}
```

### Testing Your Builder Pattern
```cpp
TEST_CASE("FrameBuilder fluent API", "[builder]") {
    auto frame = FrameBuilder<FixedFrame>()
        .with_id(0x123)
        .with_dlc(8)
        .build();
    
    REQUIRE(frame.ok());
    REQUIRE(frame.value().get_id() == 0x123);
}
```

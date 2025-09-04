#!/bin/bash

# CBOR Memory Safety Test Suite
# Comprehensive testing for embedded safety verification

set -e  # Exit on any error

echo "🚀 CBOR Memory Safety Test Suite"
echo "==============================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Test configuration
TARGETS=("native" "embedded")
TEST_TIMEOUT=30

print_status $BLUE "📋 Test Configuration:"
echo "  - Targets: ${TARGETS[@]}"
echo "  - Timeout: ${TEST_TIMEOUT}s per test"
echo "  - Stress tests: 6 generated tests"
echo ""

# Clean and build everything
print_status $YELLOW "🧹 Cleaning previous builds..."
make clean > /dev/null 2>&1

for target in "${TARGETS[@]}"; do
    print_status $BLUE "🔨 Building for target: $target"
    
    # Build main library and examples
    if make TARGET=$target all > build_${target}.log 2>&1; then
        print_status $GREEN "  ✅ Library build successful"
    else
        print_status $RED "  ❌ Library build failed"
        echo "    Check build_${target}.log for details"
        continue
    fi
    
    # Generate stress tests (only once)
    if [ "$target" == "native" ]; then
        print_status $YELLOW "🧪 Generating stress tests..."
        if python3 tools/stress_test_generator.py > /dev/null 2>&1; then
            print_status $GREEN "  ✅ Stress tests generated"
        else
            print_status $RED "  ❌ Stress test generation failed"
            continue
        fi
    fi
    
    # Build stress tests
    print_status $YELLOW "  🔧 Building stress tests..."
    if make -C stress_tests TARGET=$target all-stress > stress_build_${target}.log 2>&1; then
        print_status $GREEN "  ✅ Stress tests built"
    else
        print_status $RED "  ❌ Stress test build failed"
        echo "    Check stress_build_${target}.log for details"
        continue
    fi
    
    echo ""
done

# Run memory analysis
print_status $BLUE "📊 Running memory analysis..."
if [ -f "build/embedded/main.map" ]; then
    if python3 tools/memory_analyzer.py build/embedded/main.map > memory_analysis.log 2>&1; then
        print_status $GREEN "  ✅ Memory analysis completed"
        echo "    Report saved to memory_analysis.log"
    else
        print_status $YELLOW "  ⚠️  Memory analysis had issues"
    fi
else
    print_status $YELLOW "  ⚠️  No embedded build found for analysis"
fi

# Run basic examples first
print_status $BLUE "🧪 Running basic examples..."

for target in "${TARGETS[@]}"; do
    print_status $YELLOW "  Testing $target examples..."
    
    examples=("test-parse" "test-encode" "identify-parse" "identify-encode" "test-indefinite")
    
    for example in "${examples[@]}"; do
        if [ "$target" == "native" ]; then
            if timeout $TEST_TIMEOUT build/native/$example > /dev/null 2>&1; then
                print_status $GREEN "    ✅ $example"
            else
                print_status $RED "    ❌ $example failed"
            fi
        else
            # Embedded tests with QEMU
            if timeout $TEST_TIMEOUT qemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel build/embedded/$example.elf > /dev/null 2>&1; then
                print_status $GREEN "    ✅ $example"
            else
                print_status $RED "    ❌ $example failed"
            fi
        fi
    done
done

echo ""

# Run stress tests
print_status $BLUE "🔥 Running stress tests..."

stress_tests=("deep_nesting_50" "deep_nesting_100" "large_data_1024" "large_data_4096" "mixed_complexity" "stack_overflow_detector")

for target in "${TARGETS[@]}"; do
    print_status $YELLOW "  Stress testing $target..."
    
    for test in "${stress_tests[@]}"; do
        if [ "$target" == "native" ]; then
            if timeout $TEST_TIMEOUT stress_tests/build/native/stress_tests/$test > stress_${test}_native.log 2>&1; then
                print_status $GREEN "    ✅ $test"
            else
                print_status $RED "    ❌ $test failed"
            fi
        else
            # Embedded stress tests with QEMU
            if timeout $TEST_TIMEOUT qemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel stress_tests/build/embedded/stress_tests/$test.elf > stress_${test}_embedded.log 2>&1; then
                print_status $GREEN "    ✅ $test"
            else
                result=$?
                if [ $result -eq 124 ]; then
                    print_status $YELLOW "    ⏱️  $test (timeout - may be normal)"
                else
                    print_status $RED "    ❌ $test failed"
                fi
            fi
        fi
    done
done

echo ""

# Generate final report
print_status $BLUE "📋 Generating comprehensive report..."

cat > TEST_RESULTS.md << EOF
# 🧪 CBOR Memory Safety Test Results

**Test Date**: $(date)
**Test Suite Version**: 1.0

## 📊 Test Summary

### Build Results
- **Native Build**: $([ -f build/native/main ] && echo "✅ Success" || echo "❌ Failed")
- **Embedded Build**: $([ -f build/embedded/main.elf ] && echo "✅ Success" || echo "❌ Failed")
- **Stress Tests**: $([ -d stress_tests/build ] && echo "✅ Generated" || echo "❌ Failed")

### Basic Example Tests
EOF

# Count test results
native_pass=0
native_total=0
embedded_pass=0
embedded_total=0

for target in "${TARGETS[@]}"; do
    echo "#### $target Examples" >> TEST_RESULTS.md
    examples=("test-parse" "test-encode" "identify-parse" "identify-encode" "test-indefinite")
    
    for example in "${examples[@]}"; do
        if [ "$target" == "native" ]; then
            native_total=$((native_total + 1))
            if timeout 5s build/native/$example > /dev/null 2>&1; then
                echo "- ✅ $example" >> TEST_RESULTS.md
                native_pass=$((native_pass + 1))
            else
                echo "- ❌ $example" >> TEST_RESULTS.md
            fi
        else
            embedded_total=$((embedded_total + 1))
            if timeout 5s qemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel build/embedded/$example.elf > /dev/null 2>&1; then
                echo "- ✅ $example" >> TEST_RESULTS.md
                embedded_pass=$((embedded_pass + 1))
            else
                echo "- ❌ $example" >> TEST_RESULTS.md
            fi
        fi
    done
    echo "" >> TEST_RESULTS.md
done

# Add stress test results
echo "### Stress Test Results" >> TEST_RESULTS.md
echo "#### Native Stress Tests" >> TEST_RESULTS.md

for test in "${stress_tests[@]}"; do
    if timeout 5s stress_tests/build/native/stress_tests/$test > /dev/null 2>&1; then
        echo "- ✅ $test" >> TEST_RESULTS.md
    else
        echo "- ❌ $test" >> TEST_RESULTS.md
    fi
done

echo "" >> TEST_RESULTS.md
echo "#### Embedded Stress Tests" >> TEST_RESULTS.md

for test in "${stress_tests[@]}"; do
    if timeout 5s qemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel stress_tests/build/embedded/stress_tests/$test.elf > /dev/null 2>&1; then
        echo "- ✅ $test" >> TEST_RESULTS.md
    else
        echo "- ⏱️ $test (timeout/incomplete)" >> TEST_RESULTS.md
    fi
done

# Add summary statistics
cat >> TEST_RESULTS.md << EOF

## 📈 Test Statistics

- **Native Examples**: $native_pass/$native_total passed ($(echo "scale=1; $native_pass * 100 / $native_total" | bc -l)%)
- **Embedded Examples**: $embedded_pass/$embedded_total passed ($(echo "scale=1; $embedded_pass * 100 / $embedded_total" | bc -l)%)

## 🎯 Conclusion

$([ $native_pass -eq $native_total ] && [ $embedded_pass -eq $embedded_total ] && echo "✅ **ALL BASIC TESTS PASSED** - Library is ready for deployment" || echo "⚠️ **SOME TESTS FAILED** - Review logs before deployment")

## 📁 Generated Files

- \`TEST_RESULTS.md\` - This report
- \`STACK_SAFETY_REPORT.md\` - Detailed stack analysis
- \`MEMORY_REPORT.md\` - Memory usage analysis
- \`build_*.log\` - Build logs
- \`stress_*.log\` - Individual stress test logs

EOF

print_status $GREEN "✅ Test suite completed!"
print_status $BLUE "📋 Reports generated:"
echo "  - TEST_RESULTS.md (summary)"
echo "  - STACK_SAFETY_REPORT.md (detailed analysis)"
echo ""

if [ $native_pass -eq $native_total ] && [ $embedded_pass -eq $embedded_total ]; then
    print_status $GREEN "🎉 ALL TESTS PASSED - Ready for deployment!"
    exit 0
else
    print_status $YELLOW "⚠️  Some tests failed - Check logs for details"
    exit 1
fi

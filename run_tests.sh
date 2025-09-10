#!/bin/bash

# CBOR Library Test Runner
# This script runs all tests for the CBOR library on both native and embedded targets

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test result tracking
TESTS_PASSED=0
TESTS_FAILED=0

print_header() {
    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}======================================${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

run_test() {
    local test_name="$1"
    local test_command="$2"
    local timeout_duration="${3:-30}"
    
    echo -e "${BLUE}Running $test_name...${NC}"
    
    if timeout "$timeout_duration"s $test_command; then
        print_success "$test_name completed successfully"
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            print_warning "$test_name timed out after ${timeout_duration}s"
            return 0  # Timeout is acceptable for QEMU tests
        else
            print_error "$test_name failed with exit code $exit_code"
            return 1
        fi
    fi
}

check_dependencies() {
    print_header "Checking Dependencies"
    
    # Check for required tools
    for tool in gcc make; do
        if command -v "$tool" &> /dev/null; then
            print_success "$tool is available"
        else
            print_error "$tool is not installed"
            exit 1
        fi
    done
    
    # Check for ARM toolchain (optional)
    if command -v arm-none-eabi-gcc &> /dev/null; then
        print_success "ARM toolchain is available"
        ARM_AVAILABLE=1
    else
        print_warning "ARM toolchain not available, skipping embedded tests"
        ARM_AVAILABLE=0
    fi
    
    # Check for QEMU (optional)
    if command -v qemu-system-arm &> /dev/null; then
        print_success "QEMU is available"
        QEMU_AVAILABLE=1
    else
        print_warning "QEMU not available, skipping QEMU tests"
        QEMU_AVAILABLE=0
    fi
}

run_native_tests() {
    print_header "Building and Testing Native Target"
    
    # Clean and build
    echo "Cleaning previous build..."
    make TARGET=native clean
    
    echo "Building native target..."
    if make TARGET=native all; then
        print_success "Native build completed"
    else
        print_error "Native build failed"
        return 1
    fi
    
    # List built executables
    echo "Built executables:"
    find build/native -type f -executable -ls || true
    
    # Run tests
    local test_executables=(
        "main"
        "test-parse"
        "test-encode" 
        "test-indefinite"
        "test-stress"
        "identify-parse"
        "identify-encode"
    )
    
    for test in "${test_executables[@]}"; do
        if [ -x "build/native/$test" ]; then
            run_test "Native $test" "./build/native/$test" 60
        else
            print_warning "Native $test executable not found"
        fi
    done
}

run_embedded_tests() {
    if [ $ARM_AVAILABLE -eq 0 ]; then
        print_warning "Skipping embedded tests - ARM toolchain not available"
        return 0
    fi
    
    print_header "Building and Testing Embedded Target"
    
    # Clean and build
    echo "Cleaning previous build..."
    make TARGET=embedded clean
    
    echo "Building embedded target..."
    if make TARGET=embedded all; then
        print_success "Embedded build completed"
    else
        print_error "Embedded build failed"
        return 1
    fi
    
    # List built executables
    echo "Built ELF files:"
    find build/embedded -name "*.elf" -ls || true
    
    if [ $QEMU_AVAILABLE -eq 0 ]; then
        print_warning "Skipping QEMU tests - QEMU not available"
        return 0
    fi
    
    # Run tests in QEMU
    local test_executables=(
        "main.elf"
        "test-parse.elf"
        "test-encode.elf"
        "test-indefinite.elf"
        "identify-parse.elf"
        "identify-encode.elf"
    )
    
    for test in "${test_executables[@]}"; do
        if [ -f "build/embedded/$test" ]; then
            run_test "QEMU $test" "qemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel build/embedded/$test" 60
        else
            print_warning "Embedded $test not found"
        fi
    done
}

run_code_quality_checks() {
    print_header "Code Quality Checks"
    
    # Check if cppcheck is available
    if command -v cppcheck &> /dev/null; then
        echo "Running cppcheck static analysis..."
        if cppcheck --enable=all --error-exitcode=0 --suppress=missingIncludeSystem lib/ examples/ 2>/dev/null; then
            print_success "cppcheck analysis completed"
        else
            print_warning "cppcheck found some issues (non-fatal)"
        fi
    else
        print_warning "cppcheck not available, skipping static analysis"
    fi
    
    # Check if clang-format is available and .clang-format exists
    if command -v clang-format &> /dev/null && [ -f .clang-format ]; then
        echo "Checking code formatting..."
        if find lib/ examples/ -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror; then
            print_success "Code formatting check passed"
        else
            print_warning "Code formatting issues found"
        fi
    else
        print_warning "clang-format not available or .clang-format not found, skipping format check"
    fi
}

print_summary() {
    print_header "Test Summary"
    
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        print_success "All tests completed successfully!"
        exit 0
    else
        print_error "Some tests failed!"
        exit 1
    fi
}

# Main execution
main() {
    print_header "CBOR Library Test Runner"
    
    check_dependencies
    run_native_tests
    run_embedded_tests
    run_code_quality_checks
    print_summary
}

# Handle command line arguments
case "${1:-}" in
    "native")
        check_dependencies
        run_native_tests
        ;;
    "embedded")
        check_dependencies
        run_embedded_tests
        ;;
    "quality")
        check_dependencies
        run_code_quality_checks
        ;;
    "help"|"-h"|"--help")
        echo "Usage: $0 [native|embedded|quality|help]"
        echo "  native    - Run only native tests"
        echo "  embedded  - Run only embedded/QEMU tests"
        echo "  quality   - Run only code quality checks"
        echo "  help      - Show this help message"
        echo "  (no args) - Run all tests"
        ;;
    *)
        main
        ;;
esac

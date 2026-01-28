#!/bin/bash

# Build script for Temperature GUI Application
# Supports multiple build methods

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions
print_header() {
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    print_header "Checking Prerequisites"
    
    # Check for Qt5
    if ! command -v qmake &> /dev/null; then
        print_error "Qt5 not found. Please install Qt5:"
        echo "  macOS: brew install qt5"
        echo "  Ubuntu/Debian: sudo apt-get install qt5-qmake qt5-default"
        echo "  Windows: Download from https://www.qt.io"
        exit 1
    fi
    print_info "Qt5 found: $(qmake --version | head -n 1)"
    
    # Check for Qwt
    QWT_FOUND=false
    
    # Try pkg-config first
    if pkg-config --exists Qwt6 2>/dev/null; then
        QWT_FOUND=true
    fi
    
    # Try standard locations
    if [ "$QWT_FOUND" = false ] && ls /usr/local/lib/libqwt* &>/dev/null; then
        QWT_FOUND=true
    fi
    
    # Try Homebrew locations (macOS)
    if [ "$QWT_FOUND" = false ] && [ -d "$(brew --prefix qwt 2>/dev/null)/lib" ]; then
        QWT_FOUND=true
    fi
    
    # Try /opt paths
    if [ "$QWT_FOUND" = false ] && ls /opt/*/lib/libqwt* &>/dev/null 2>&1; then
        QWT_FOUND=true
    fi
    
    if [ "$QWT_FOUND" = false ]; then
        print_warning "Qwt not found in standard locations"
        echo "  Install with:"
        echo "  macOS: brew install qwt"
        echo "  Ubuntu/Debian: sudo apt-get install libqwt-qt5-dev"
        echo "  Windows: Download from https://qwt.sourceforge.io"
        read -p "Continue anyway? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        print_info "Qwt found"
    fi
    
    # Check for build tools
    if ! command -v make &> /dev/null; then
        print_error "make not found"
        exit 1
    fi
    print_info "Build tools found"
}

# Method 1: qmake
build_with_qmake() {
    print_header "Building with qmake"
    
    qmake temperature_gui.pro
    make clean
    make
    
    print_info "Build successful!"
    print_info "Run with: ./temperature_gui"
}

# Method 2: CMake
build_with_cmake() {
    print_header "Building with CMake"
    
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found"
        echo "Install with:"
        echo "  macOS: brew install cmake"
        echo "  Ubuntu/Debian: sudo apt-get install cmake"
        exit 1
    fi
    
    mkdir -p build
    cd build
    cmake ..
    make
    cd ..
    
    print_info "Build successful!"
    print_info "Run with: ./build/temperature_gui"
}

# Method 3: Manual Makefile
build_with_makefile() {
    print_header "Building with Makefile"
    
    make clean
    make
    
    print_info "Build successful!"
    print_info "Run with: ./temperature_gui"
}

# Show usage
show_usage() {
    cat << EOF
Temperature GUI Application - Build Script

Usage: $0 [METHOD]

Methods:
  qmake    - Build using Qt qmake (default)
  cmake    - Build using CMake
  makefile - Build using custom Makefile

Examples:
  $0           # Uses qmake
  $0 cmake     # Uses CMake
  $0 makefile  # Uses Makefile

EOF
}

# Main script
main() {
    # Change to script directory
    cd "$(dirname "$0")"
    
    # Check if help requested
    if [[ "$1" == "-h" || "$1" == "--help" ]]; then
        show_usage
        exit 0
    fi
    
    # Check prerequisites
    check_prerequisites
    
    # Determine build method
    METHOD="${1:-qmake}"
    
    case "$METHOD" in
        qmake)
            build_with_qmake
            ;;
        cmake)
            build_with_cmake
            ;;
        makefile)
            build_with_makefile
            ;;
        *)
            print_error "Unknown build method: $METHOD"
            show_usage
            exit 1
            ;;
    esac
    
    echo ""
    print_header "Build Complete"
    echo -e "${GREEN}The GUI application has been built successfully!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Ensure simulator and logger are running:"
    echo "   cd src && ./simulator | ./logger"
    echo ""
    echo "2. In another terminal, run the GUI:"
    if [[ "$METHOD" == "cmake" ]]; then
        echo "   ./build/temperature_gui"
    else
        echo "   ./temperature_gui"
    fi
}

main "$@"

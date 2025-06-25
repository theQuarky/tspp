#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Error handling function
handle_error() {
    echo -e "${RED}Error: $1${NC}"
    exit 1
}

echo -e "${BLUE}TSPP Build Script${NC}"
echo -e "${YELLOW}Starting build process...${NC}"

# Check if build directory exists
if [ -d "build" ]; then
    echo -e "${YELLOW}Removing old build directory...${NC}"
    rm -rf build || handle_error "Failed to remove old build directory"
fi

# Create new build directory
echo -e "${YELLOW}Creating new build directory...${NC}"
mkdir build || handle_error "Failed to create build directory"
cd build || handle_error "Failed to enter build directory"

# Configure CMake
echo -e "${YELLOW}Configuring CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON || handle_error "CMake configuration failed"

# Get number of CPU cores for parallel build
if [ "$(uname)" == "Darwin" ]; then
    # macOS
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    # Linux and others
    NUM_CORES=$(nproc)
fi

# Build project using all available cores
echo -e "${YELLOW}Building project using ${NUM_CORES} cores...${NC}"
cmake --build . -- -j"${NUM_CORES}" || handle_error "Build failed"

echo -e "${GREEN}Build completed successfully!${NC}"

# Copy compile_commands.json to root for tools like clangd
if [ -f "compile_commands.json" ]; then
    echo -e "${YELLOW}Copying compile_commands.json to project root...${NC}"
    cp compile_commands.json .. || handle_error "Failed to copy compile_commands.json"
fi

# Run a test build
echo -e "${YELLOW}Running test build...${NC}"
cd .. || handle_error "Failed to return to project root"
./build/src/tspp --version || echo -e "${YELLOW}Note: --version not implemented yet${NC}"

echo -e "${GREEN}All done! Binary located at ./build/src/tspp${NC}"
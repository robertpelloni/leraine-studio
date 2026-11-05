# Deployment & Build Guide

## Prerequisites

### Linux (Ubuntu / Debian)
Install system packages:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libsfml-dev \
    libyaml-cpp-dev \
    zlib1g-dev \
    libgl-dev
```

### Windows (MSVC)
- CMake 3.16+
- Visual Studio 2022 / Build Tools with C++17 support
- SFML 2.5+ / 2.6+
- yaml-cpp

## Building the Project

### 1. Configure CMake
```bash
mkdir build
cd build
cmake -DIMGUI_SFML_FIND_SFML=OFF ..
```

### 2. Build Core Library and Test Runner
```bash
make -j$(nproc) test_runner
ctest --output-on-failure
```

### 3. Build Main Executable
```bash
make -j$(nproc) leraine-studio
```

The resulting binaries will be placed in the `binaries/` directory alongside asset folders and dependencies.

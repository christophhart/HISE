#!/bin/bash

# Optimized install script for HISE VST development environment on macOS ARM (Apple Silicon)

# Exit on any error
set -e

# Define variables
HISE_REPO="https://github.com/christophhart/HISE.git"
HISE_DIR="$HOME/hise"
SDK_DIR="$HISE_DIR/tools/SDK"
VST3_SDK_URL="https://download.steinberg.net/sdk_downloads/vst-sdk_3.7.1_build-50_2020-11-17.zip"
ASIO_SDK_URL="https://www.steinberg.net/asiosdk"
VST3_SDK_ZIP="vst-sdk_3.7.1_build-50_2020-11-17.zip"
SDK_ZIP_PASSWORD="4321"
LOG_FILE="$HOME/hise_install.log"
TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")

# Function to log errors
log_error() {
    echo "[$TIMESTAMP] ERROR: $1" | tee -a "$LOG_FILE"
}

# Function to handle errors and exit
handle_error() {
    log_error "$1"
    echo "Installation failed. Check $LOG_FILE for details."
    echo "You may need to manually install missing dependencies or check network connectivity."
    exit 1
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1 || { log_error "$1 not found."; return 1; }
}

# Initialize log file
echo "[$TIMESTAMP] Starting HISE installation on macOS ARM" > "$LOG_FILE"

# Check for Homebrew and install if not present
if ! command_exists brew; then
    echo "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to install Homebrew. Check internet connection or Homebrew website."
    }
    # Add Homebrew to PATH
    eval "$(/opt/homebrew/bin/brew shellenv)" || {
        handle_error "Failed to configure Homebrew PATH."
    }
    echo "Homebrew installed successfully."
else
    echo "Homebrew found. Updating..."
    brew update >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to update Homebrew."
    }
fi

# Install dependencies in one command
echo "Installing dependencies via Homebrew..."
brew install cmake git libpng freetype llvm pkg-config curl unzip >> "$LOG_FILE" 2>&1 || {
    handle_error "Failed to install dependencies. Ensure Homebrew is working and try 'brew doctor'."
}

# Check for Xcode Command Line Tools
if ! xcode-select -p >/dev/null 2>&1; then
    echo "Installing Xcode Command Line Tools..."
    xcode-select --install >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to initiate Xcode Command Line Tools installation."
    }
    echo "Please follow the prompts to install Xcode Command Line Tools, then rerun this script."
    exit 0
fi

# Create HISE directory with permissions
mkdir -p "$HISE_DIR" || {
    handle_error "Failed to create HISE directory at $HISE_DIR. Check permissions."
}
chmod -R u+rw "$HISE_DIR" || {
    handle_error "Failed to set permissions for $HISE_DIR."
}

# Clone or update HISE repository
if [ ! -d "$HISE_DIR/.git" ]; then
    echo "Cloning HISE repository to $HISE_DIR..."
    git clone "$HISE_REPO" "$HISE_DIR" >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to clone HISE repository. Check internet or GitHub access."
    }
else
    echo "HISE directory exists. Pulling latest changes..."
    cd "$HISE_DIR"
    git pull >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to update HISE repository."
    }
fi

# Create SDK directory
mkdir -p "$SDK_DIR" || {
    handle_error "Failed to create SDK directory at $SDK_DIR."
}

# Download and extract VST3 SDK
if [ ! -d "$SDK_DIR/VST3 SDK" ]; then
    echo "Downloading VST3 SDK..."
    curl -L "$VST3_SDK_URL" -o "$SDK_DIR/$VST3_SDK_ZIP" >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to download VST3 SDK from $VST3_SDK_URL."
    }
    # Basic file size check
    if [ ! -s "$SDK_DIR/$VST3_SDK_ZIP" ]; then
        handle_error "Downloaded VST3 SDK file is empty or corrupted."
    fi
    echo "Extracting VST3 SDK..."
    unzip -P "$SDK_ZIP_PASSWORD" "$SDK_DIR/$VST3_SDK_ZIP" -d "$SDK_DIR" >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to extract VST3 SDK. Check if the password or file is correct."
    }
    mv "$SDK_DIR/VST_SDK/VST3_SDK" "$SDK_DIR/VST3 SDK" >> "$LOG_FILE" 2>&1 || {
        handle_error "Failed to move VST3 SDK to $SDK_DIR/VST3 SDK."
    }
else
    echo "VST3 SDK already exists at $SDK_DIR/VST3 SDK."
fi

# ASIO SDK instructions
if [ ! -d "$SDK_DIR/ASIOSDK2.3" ]; then
    echo "Note: ASIO SDK is primarily needed for Windows standalone applications."
    echo "Download it manually from $ASIO_SDK_URL and extract to $SDK_DIR/ASIOSDK2.3 if needed."
else
    echo "ASIO SDK already exists at $SDK_DIR/ASIOSDK2.3."
fi

# Check and configure Projucer
PROJUCER_PATH="$HISE_DIR/tools/projucer/Projucer.app/Contents/MacOS/Projucer"
if [ ! -f "$PROJUCER_PATH" ]; then
    handle_error "Projucer not found at $PROJUCER_PATH. Ensure HISE repository is correctly cloned."
fi
chmod +x "$PROJUCER_PATH" >> "$LOG_FILE" 2>&1 || {
    handle_error "Failed to set executable permissions for Projucer."
}

# Completion message with compilation instructions
echo "[$TIMESTAMP] HISE setup complete! Log saved to $LOG_FILE" | tee -a "$LOG_FILE"
echo "To compile HISE:"
echo "1. Open Projucer: open $HISE_DIR/tools/projucer/Projucer.app"
echo "2. Load project: $HISE_DIR/projects/standalone/HISE Standalone.jucer"
echo "3. Click 'Save Project and Open in IDE' to open in Xcode."
echo "4. In Xcode, select 'Release' configuration and build."
echo "5. Alternatively, compile with CMake:"
echo "   cd $HISE_DIR/projects/standalone/Builds/MacOSX"
echo "   cmake . -Bbuild -DCMAKE_BUILD_TYPE=Release"
echo "   cmake --build build --config Release"
echo "6. Compiled HISE executable will be in $HISE_DIR/projects/standalone/Builds/MacOSX/build."
echo "For help, visit https://docs.hise.audio or https://forum.hise.audio."
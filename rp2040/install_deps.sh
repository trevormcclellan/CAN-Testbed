#!/bin/bash

# Define installation directory in the user's home directory
INSTALL_DIR="$HOME/bin"
ARDUINO_CLI_BIN="$INSTALL_DIR/arduino-cli"

# Install arduino-cli
echo "🔧 Installing arduino-cli..."
if ! command -v arduino-cli &> /dev/null; then
    # Download and install arduino-cli
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

    # Move the arduino-cli binary from the current bin folder to the home directory's bin folder
    if [ -f "./bin/arduino-cli" ]; then
        mkdir -p "$INSTALL_DIR"
        mv ./bin/arduino-cli "$ARDUINO_CLI_BIN"

        # Clean up empty bin directory from installer
        rmdir ./bin 2>/dev/null || echo "⚠️ Could not remove ./bin directory (may not be empty)."

        # Make sure $INSTALL_DIR is in the user's PATH
        if ! echo "$PATH" | grep -q "$INSTALL_DIR"; then
            echo "🔧 Adding $INSTALL_DIR to PATH"
            echo "export PATH=\"$INSTALL_DIR:\$PATH\"" >> ~/.bashrc
            source ~/.bashrc
        fi
    else
        echo "Error: arduino-cli binary not found in the expected location."
        exit 1
    fi
else
    echo "arduino-cli is already installed."
fi

# Initialize arduino-cli (if it hasn't been set up before)
echo "🔧 Initializing arduino-cli..."
$ARDUINO_CLI_BIN config init

# Set unsafe install option
echo "🔧 Enabling 'enable_unsafe_install' in arduino-cli.yaml..."
$ARDUINO_CLI_BIN config set library.enable_unsafe_install true

# Add Longan-RP2040 board manager URL
BOARD_MANAGER_URL="https://raw.githubusercontent.com/Longan-Labs/Longan-RP2040/main/package_rp2040_index.json"
echo "🔧 Adding Longan-RP2040 board manager URL to arduino-cli..."
$ARDUINO_CLI_BIN config add board_manager.additional_urls "$BOARD_MANAGER_URL"

# Check if .arduinoIDE/arduino-cli.yaml exists
ARDUINO_IDE_CONFIG="$HOME/.arduinoIDE/arduino-cli.yaml"
if [ -f "$ARDUINO_IDE_CONFIG" ]; then
    echo "🔧 Found .arduinoIDE/arduino-cli.yaml. Adding board manager URL there as well..."
    # Add the Longan-RP2040 board manager URL to .arduinoIDE/arduino-cli.yaml
    $ARDUINO_CLI_BIN config --config-file "$ARDUINO_IDE_CONFIG" add board_manager.additional_urls "$BOARD_MANAGER_URL"
fi

# Update arduino-cli core index
echo "🔧 Updating arduino-cli core index..."
$ARDUINO_CLI_BIN core update-index

# Install libraries
echo "🔧 Installing libraries..."

# Install ArduinoJson
$ARDUINO_CLI_BIN lib install "ArduinoJson" || { echo "Failed to install ArduinoJson"; exit 1; }

# Install ArduinoQueue
$ARDUINO_CLI_BIN lib install "ArduinoQueue" || { echo "Failed to install ArduinoQueue"; exit 1; }

# Install library from GitHub using --git-url
LIBRARY_GIT_URL="https://github.com/Longan-Labs/Aruino_CAN_BUS_MCP2515.git"
echo "🔧 Installing library from Git URL: $LIBRARY_GIT_URL"
$ARDUINO_CLI_BIN lib install --git-url "$LIBRARY_GIT_URL" || { echo "Failed to install library from Git URL"; exit 1; }

echo "✅ All libraries and board manager URL added successfully."

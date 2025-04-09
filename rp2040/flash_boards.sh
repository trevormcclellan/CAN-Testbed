#!/bin/bash

# Check if sketch path is provided
if [ -z "$1" ]; then
    echo "Usage: $0 /full/path/to/your_sketch"
    exit 1
fi

# Prompt to ensure boards are powered on or off as needed
echo "Please power on all boards that you want to flash."
echo "Please power off any boards that you do not want to flash."
echo "Press Enter when you are ready to proceed..."
read -r

SKETCH_PATH="$1"
FQBN="longan-rp2040:longan-rp2040:canbed2040" 
TARGET_VID="2e8a"
TARGET_PID="000a"

# Compile once
echo "🔧 Compiling sketch at $SKETCH_PATH..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH_PATH"
if [ $? -ne 0 ]; then
    echo "❌ Compile failed"
    exit 1
fi

# Flash all matching devices
echo "🔍 Searching for devices with VID:PID = $TARGET_VID:$TARGET_PID"
for dev in /dev/ttyACM*; do
    VID=$(udevadm info -a -n "$dev" | grep "idVendor" | head -n1 | awk -F '==' '{print $2}' | tr -d '" ')
    PID=$(udevadm info -a -n "$dev" | grep "idProduct" | head -n1 | awk -F '==' '{print $2}' | tr -d '" ')

    if [[ "$VID" == "$TARGET_VID" && "$PID" == "$TARGET_PID" ]]; then
        echo "🚀 Uploading to $dev (VID:PID = $VID:$PID)"
        arduino-cli upload -p "$dev" --fqbn "$FQBN" "$SKETCH_PATH"
    else
        echo "⏭️  Skipping $dev (VID:PID = $VID:$PID)"
    fi
done

wait
echo "✅ Done flashing all matching RP2040 boards."

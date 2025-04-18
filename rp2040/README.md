# RP2040 Firmware Directory

This directory contains all Arduino sketches and scripts needed for programming the CANBed RP2040 development boards used in the CANBench testbed.

## Directory Contents

### `CANBed/`
This folder contains the **base CANBench firmware**, written in Arduino C++. This is the default firmware that should be flashed to the CANBed RP2040 boards for standard operation of the testbed.

### `examples/`
This folder contains **example Arduino sketches** demonstrating extended or alternative functionality on the CANBed boards:

- **CANBed_LCD** – Displays steering angle information on a connected LCD screen.
- **CANBed_button** – Sends CAN messages to trigger hazard lights when a button is pressed.
- **CANBed_encryption** – A proof-of-concept security control. This firmware decrypts CAN messages as they are received.  
  > This sketch is designed to work with `backend_encryption.py` in the `Backend/` directory.
- **DoS** – Demonstrates a denial-of-service attack by flooding the CAN bus with traffic.

## Scripts

### `install_deps.sh`
Installs the [Arduino CLI](https://arduino.github.io/arduino-cli/) and all required libraries for building and uploading sketches to the CANBed RP2040 boards.

```bash
./install_deps.sh
```

### `flash_boards.sh`
Automatically flashes all **connected** CANBed RP2040 boards with the specified sketch using the Arduino CLI.

**Usage:**
```bash
./flash_boards.sh /full/path/to/your_sketch
```

Example:
```bash
./flash_boards.sh $PWD/CANBed
```

> Make sure no other application (including the web dashboard) is using the serial ports while flashing.

---

For a streamlined setup experience, use the scripts above instead of the manual Arduino IDE method. Manual instructions are still available in the main project wiki.


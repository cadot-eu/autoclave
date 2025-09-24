# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

This is an Arduino-based pressure cooker/autoclave controller system with safety features. The main program (`programme.ino`) controls a heating element via relay based on water detection, pressure monitoring, and timer management.

## Hardware Architecture

### Core Components
- **Mega32U4** (Arduino compatible microcontroller)
- **TM1637** 4-digit display (CLK_PIN=2, DIO_PIN=3)
- **Pressure sensor** 0-30 PSI (A5 pin, 0.5-4.5V analog output)
- **Water detection** dual probe system (A0 pin)
- **Relay control** for 220V heating element (PIN_RELAY=9)
- **User interface** increment/decrement buttons (pins 11/12)

### Safety Systems
1. **Water Detection**: Dual stainless steel probes - system stops immediately if no water detected
2. **Pressure Control**: Three-level safety (configurable threshold, 15 PSI forced stop, 21 PSI emergency)
3. **Timer Safety**: Automatic shutdown, prevents negative timer values
4. **Electrical Safety**: Normally-open relay, galvanic isolation

## Development Commands

### Compilation and Upload
```bash
# Using Arduino IDE (GUI)
arduino /home/michael/git/autoclave/programme/programme.ino

# For command-line development, install arduino-cli:
# snap install arduino-cli --classic
# arduino-cli compile --fqbn arduino:avr:leonardo programme.ino
# arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:leonardo programme.ino
```

### Serial Monitor
```bash
# View serial output (115200 baud)
screen /dev/ttyACM0 115200
# or
minicom -D /dev/ttyACM0 -b 115200

# List available serial ports
ls /dev/ttyACM* /dev/ttyUSB*
```

## Code Architecture

### Operating Modes
- **DEBUG_MODE** (line 25): Set `true` for testing with lower pressure thresholds (0.04-0.05 MPa vs 0.15-0.16 MPa)
- **Production Mode**: Real operational thresholds for actual pressure cooking

### State Management
- **waitingPressure**: Pump running but waiting for pressure to reach threshold before starting timer
- **heatingPaused**: Heating element paused due to pressure regulation (hysteresis control)
- **pumpRunning**: Main operational state

### Key Functions
- `startPump()`: Activates relay and enters pressure-waiting state
- `stopPump()`: Emergency stop with complete state reset
- `finishCycle()`: Normal cycle completion with countdown display
- `displayTimePressure()`: Shows format MMPP (minutes + pressure in centibars)

### Safety Logic
1. **Water check first**: No water = immediate shutdown
2. **Button control**: Only when pump stopped
3. **Pressure regulation**: Hysteresis-based heating control during operation
4. **Timer management**: Only starts after pressure threshold reached

## Critical Safety Notes

⚠️ **This system controls a 220V heating element in a pressure vessel - improper modifications can be fatal**

- All pressure thresholds are in MPa (0.15 MPa ≈ 1.5 bar ≈ 22 PSI)
- Relay logic is inverted: HIGH = OFF, LOW = ON
- Serial communication should work without USB connection for safety
- Never disable water detection or pressure safety systems

## Testing and Debugging

### Debug Mode Testing
Set `DEBUG_MODE = true` for:
- Lower pressure thresholds (safer for testing)
- Verbose serial output (1Hz update rate)
- Relay state monitoring

### Production Validation Checklist
- [ ] Water detection stops heating immediately
- [ ] Pressure regulation cycles correctly
- [ ] Timer accuracy verified
- [ ] Emergency pressure stop (>21 PSI equivalent)
- [ ] Power cycle behavior safe

## File Structure

- `programme.ino`: Main controller logic (single file Arduino sketch)
- `../README.MD`: Hardware wiring diagrams and safety documentation
- `../schemas/`: Electrical schematic images
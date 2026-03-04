# TurntableDrive – Arduino TMC2209 Motor Control

## Project Description
This project controls a stepper motor via a **TMC2209 driver** using an Arduino.
It is designed to precisely drive a turntable at two fixed speeds (approx. 33 RPM and 45 RPM).

## Stepper vs. Classical Drive Comparison

For a detailed audiophile comparison of stepper motor vs. classical platter drives, see [docs/STEP_vs_CLASSICAL.md](./docs/STEP_vs_CLASSICAL.md).

## Technical Setup

The project uses **SoftwareSerial** for UART communication with the TMC2209 and allows **fine speed adjustment via a potentiometer**.
LEDs indicate whether the target speed has been reached.

---

## Features
- Stepper motor control via TMC2209
- Two selectable speeds (33 RPM / 45 RPM) via digital switch
- Fine speed adjustment with a potentiometer
- Status indication via green/red LEDs
- Smooth acceleration and deceleration of motor speed
- UART communication via SoftwareSerial
- Standby and hold current control for protection

---

## Hardware
- Arduino (Uno, Nano, or compatible)
- Stepper motor (0.9° or 1.8° step angle, adjusted for microsteps)
- TMC2209 stepper driver
- Switch for speed selection
- Potentiometer for fine speed adjustment
- LEDs (green/red) for status
- Wiring according to pinout:

| Function            | Arduino Pin |
|--------------------|------------|
| Enable (EN)         | 2          |
| Direction (DIR)     | 3          |
| Step (STEP)         | 4          |
| Speed Switch        | 8          |
| UART RX (SoftSerial)| 6          |
| UART TX (SoftSerial)| 7          |
| Green LED           | 11         |
| Red LED             | 9          |

---

## Software / Libraries
- [SoftwareSerial](https://www.arduino.cc/en/Reference/softwareSerial)
- [Streaming](https://www.arduino.cc/reference/en/libraries/streaming/)
- [TMC2209 Arduino Library](https://github.com/janelia-arduino/TMC2209)

---

## Installation
1. Open Arduino IDE
2. Load the sketch `Motorsteuerung_tmc2209_RC1.ino`
3. Install required libraries:
   - `SoftwareSerial`
   - `Streaming`
   - `TMC2209`
4. Wire the components according to the pinout table
5. Set baud rate to 115200
6. Upload the sketch to your Arduino

---

## Usage
- Switch HIGH → 33 RPM
- Switch LOW → 45 RPM
- LEDs indicate when the target speed is reached
- Potentiometer connected to A5 allows fine speed adjustment

```text
Green LED = Target speed reached
Red LED   = Target speed not yet reached
```

- The sketch runs continuously (`loop()`) and automatically adjusts the speed

---

## Notes
- `RUN_CURRENT_PERCENT` may need adjustment depending on the motor and power supply to avoid overheating
- Microsteps per step: 64 (optimized for 0.9° motors)
- Internal 12 MHz clock calculation: `microsteps_per_second = microsteps_per_period * 0.715`
- Smooth acceleration/deceleration is controlled by `accel` and `speedChangeDelay`
- Potentiometer reading is averaged to reduce noise, with a small deadzone around mid-value

---

## License
MIT License – free to use, modify, and distribute.


# ESP32 RC Racer 🏁

A dual-control wireless RC car built with ESP32, controlled via a
physical joystick remote (ESP-NOW) or phone web browser (WiFi).

---

## Features

- Dual control — physical remote OR phone web browser
- ESP-NOW protocol — ~1ms latency, ~200m range
- WiFi web controller — open in any phone browser
- 4x N20 motors with differential steering
- Dual L298N motor drivers
- OLED display on remote — shows throttle, steering, signal strength
- Failsafe — car stops automatically if signal lost
- Sport / Safe speed modes
- Turbo boost — hold right joystick button
- Horn — hold left joystick button

---

## Hardware

### Remote Controller
| Component | Quantity |
|---|---|
| ESP32 DevKit V1 | 1 |
| KY-023 Joystick | 2 |
| SSD1306 OLED 0.96" | 1 |
| 7.4V LiPo Battery | 1 |
| AMS1117-3.3V Regulator | 1 |
| PCB + Jumper Wires | — |

### RC Car
| Component | Quantity |
|---|---|
| ESP32 DevKit V1 | 1 |
| L298N Motor Driver | 2 |
| N20 Motor 6V | 4 |
| 7.4V LiPo Battery | 1 |

---

## Wiring

### Remote — ESP32 Pin Map
| Component | Pin | ESP32 |
|---|---|---|
| Left Joystick VRY | Throttle | D35 |
| Left Joystick VRX | Aux | D34 |
| Left Joystick SW | Horn | D32 |
| Right Joystick VRX | Steering | D33 |
| Right Joystick VRY | Aux | D25 |
| Right Joystick SW | Turbo | D26 |
| OLED SDA | I2C Data | D21 |
| OLED SCL | I2C Clock | D22 |
| AMS1117 OUT | Power | 3V3 |

### Car — L298N Driver 1 (Right Motors)
| L298N | ESP32 |
|---|---|
| ENA | D5 |
| IN1 | D18 |
| IN2 | D19 |
| ENB | D23 |
| IN3 | D25 |
| IN4 | D26 |

### Car — L298N Driver 2 (Left Motors)
| L298N | ESP32 |
|---|---|
| ENA | D27 |
| IN1 | D32 |
| IN2 | D33 |
| ENB | D21 |
| IN3 | D22 |
| IN4 | D4 |

### Car — Power
| From | To |
|---|---|
| Battery + | L298N 12V |
| Battery − | L298N GND |
| L298N 5V out | ESP32 VIN |
| L298N GND | ESP32 GND |

---

## Software

### Libraries Required
- Adafruit SSD1306
- Adafruit GFX
- WiFi (built-in)
- ESP-NOW (built-in)
- WebServer (built-in)

### Files
| File | Description |
|---|---|
| remote_controller.ino | Flash to remote ESP32 |
| car_receiver.ino | Flash to car ESP32 |

---

## Setup

### Step 1 — Flash Car ESP32
- Open car_receiver.ino in Arduino IDE
- Select board: ESP32 Dev Module
- Upload to car ESP32
- Open Serial Monitor at 115200
- Copy the MAC address printed

### Step 2 — Flash Remote ESP32
- Open remote_controller.ino
- Find this line:
```cpp
uint8_t CAR_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
```
- Replace with your car MAC address
- Upload to remote ESP32

### Step 3 — Power Up
- Power car → wait 2 seconds
- Power remote → OLED shows ONLINE
- Drive!

---

## Web Controller (Phone)

1. Connect phone WiFi to **RC_CAR**
2. Password: **12345678**
3. Open Chrome browser
4. Go to **192.168.4.1**
5. Use on-screen joysticks to drive

---

## Controls

### Physical Remote
| Control | Action |
|---|---|
| Left joystick up/down | Throttle forward/reverse |
| Right joystick left/right | Steering |
| Left joystick push | Horn |
| Right joystick push | Turbo boost |

### Web Controller
| Control | Action |
|---|---|
| Left joystick | Throttle |
| Right joystick | Steering |
| STOP button | Emergency stop |

---

## Protocol

| Spec | Value |
|---|---|
| Protocol | ESP-NOW |
| Latency | ~1ms |
| Range | ~200m |
| Packet rate | 50Hz |
| Packet size | 9 bytes |

---

## Troubleshooting

| Problem | Fix |
|---|---|
| OLED blank | Check SDA=D21 SCL=D22, try address 0x3D |
| OLED shows SEARCH | Wrong MAC address in remote code |
| Motors spin at boot | Add 10kΩ pulldown on IN pins |
| Car not moving | Check L298N power and ENA/ENB jumpers |
| Motors going wrong direction | Swap IN1/IN2 or IN3/IN4 wires |
| ESP32 very hot | Power via VIN not 3V3, add heatsink |
| Upload failed | Hold BOOT button while uploading |
| Web page not loading | Connect to RC_CAR WiFi first |

---

## Built By

- **Project:** ESP32 RC Racer
- **Event:** Robo Race
- **Controller:** ESP32 + ESP-NOW + WiFi
- **Motors:** 4x N20 6V 2000RPM
- **Drivers:** 2x L298N

---

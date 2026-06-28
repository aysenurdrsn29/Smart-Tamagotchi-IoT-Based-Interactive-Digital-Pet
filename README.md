# Smart Tamagotchi (ESP32)

A modern, IoT-based take on the classic digital pet, built entirely in **C++** using Object-Oriented Programming (OOP) principles. 

An ESP32-based interactive digital companion that reacts to its environment in real time — light, sound, motion, and touch each trigger a different emotional state, displayed as an animated cat face on a TFT screen. This little companion reacts to its real-world environment in real-time. It gets scared by loud noises, squints when it's too bright, falls asleep in the dark, and purrs when you pet it.

### States
 
| State | Trigger | Face | LED Aura |
|-------|---------|------|----------|
| **Normal** | Default (idle > 5 min) | Open eyes, slow blink | Dim amber breath |
| **Happy** | Touch detected | ^w^ smile + sparkles | Pulsing pink |
| **Scared** | Loud sound (> threshold) | Wide eyes, shaking | Fast red strobe |
| **Dazzled** | Bright light (LDR > 4000) | Squinting + sweat drop | White pulse |
| **Sleep** | Dark room / idle 15 min | Closed eyes, ZZZ | Night lavender |
 
After 10 seconds in Sleep, the device enters **ESP32 deep sleep** (~42 µA). A PIR motion event wakes it back up via hardware interrupt.

### Core Features
* **Finite State Machine (FSM):** Smoothly transitions between 5 emotional states (Normal, Happy, Scared, Dazzled, Sleep) without blocking the main loop.
* **Non-Blocking Architecture:** Uses hardware timers (`millis()`) to handle sensor reads, TFT screen rendering, and haptic motor feedback simultaneously.
* **Smart Power Management:** Implements a two-phase deep sleep pipeline, dropping active current to below 50 µA while saving interaction history in RTC persistent memory.

### Hardware Setup
* **Microcontroller:** ESP32-WROOM-32
* **Sensors:** Capacitive Touch (HW-139), Acoustic (MAX9814), Ambient Light (LDR), Proximity (PIR)
* **Outputs:** 1.44" TFT LCD (SPI), WS2812B LED Ring, Haptic Vibration Motor

### Special Modes
 
**Diary Mode** Press the Boot button (GPIO 0) to overlay live sensor readings on the display: sound level (ADC + dB estimate), LDR value, PIR and touch state, current uptime, and RTC counters.
 
**Demo Mode** Hold the Demo button (GPIO 25) while powering on, or short-press it during operation, to shorten the idle sleep timeout to 30 seconds. Long-press (2 s) forces immediate deep sleep.

### File Structure

```
SmartTamagotchi/
├── SmartTamagotchi.ino   # Main loop + state machine
├── faces.h               # TFT face drawing functions
└── aura.h                # LED aura + motor patterns
```

### Libraries
 
Install via Arduino Library Manager:
 
- `Adafruit GFX Library`
- `Adafruit ST7735 and ST7789 Library`
- `FastLED`

This project demonstrates practical skills in **embedded C++**, **sensor integration**, and **hardware-level power optimization**.
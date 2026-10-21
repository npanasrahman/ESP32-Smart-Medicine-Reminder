# ESP32 Smart Medicine Reminder System

## Project Description

This project is an ESP32-based smart medicine
reminder system using the Blynk IoT platform.

It reminds users to take medicines at scheduled
times using a buzzer, LED, LCD display, and
mobile notifications.

## Features

- Scheduled medicine reminders
- 16x2 I2C LCD display
- Buzzer alarm
- LED indicator
- Physical push button
- Blynk mobile notifications
- Automatic alarm shutoff after 30 seconds
- NTP time synchronization

## Hardware Requirements

- ESP32 Development Board
- 16x2 I2C LCD
- Buzzer
- LED
- Current-limiting resistor
- Push button
- Jumper wires
- Breadboard

## Pin Connections

| Component | ESP32 Pin |
|---|---|
| Buzzer | GPIO 25 |
| LED | GPIO 26 |
| Push Button | GPIO 19 |
| LCD SDA | GPIO 21 |
| LCD SCL | GPIO 22 |

## Medicine Schedule

- Paracetamol: 09:00 AM
- Nazel drops: 02:00 PM
- Dolo: 09:00 PM

## Software Requirements

- Arduino IDE
- ESP32 Board Support
- Blynk IoT
- LiquidCrystal_I2C Library

## How It Works

The ESP32 checks the medicine schedule.
When a scheduled time arrives, the buzzer
sounds, the LED lights up, and the LCD
displays the medicine name.

A Blynk notification is sent when the
internet and Blynk connection are available.

The alarm can be stopped using the physical
button or the Blynk mobile application.

## Security

Do not upload Wi-Fi passwords or Blynk
authentication tokens to a public repository.

## Author

Anas Rahman

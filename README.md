# Smart Pill Box System Based on STM32

This project is a smart pill box system built on `STM32F103`. It combines medicine reminder, health data acquisition, local display, and Bluetooth serial interaction in one embedded application.

## Overview

The system uses STM32 as the main controller and integrates multiple sensors and peripheral modules to complete:

- medicine reminder and confirmation
- heart rate and blood oxygen monitoring
- ambient temperature and humidity monitoring
- body temperature monitoring
- real-time clock display and adjustment
- buzzer, LED, and voice alarm output
- Bluetooth command-based parameter configuration

## Main Features

- `OLED` multi-page display
- `MAX30102` heart rate and SpO2 acquisition
- `DHT11` ambient temperature and humidity acquisition
- `DS18B20` body temperature acquisition
- `DS1302` real-time clock
- timed medicine reminder
- medicine stock deduction after confirmation
- threshold-based alarm logic
- voice playback through `JQ8400`
- Bluetooth serial command control

## Hardware Modules

- `STM32F103`
- `OLED`
- `MAX30102`
- `DHT11`
- `DS18B20`
- `DS1302`
- `JQ8400`
- buzzer
- LED
- keys

## Software Structure

- `User/`
  Main application entry and interrupt-related files
- `Hardware/`
  Peripheral drivers and business modules
- `System/`
  Basic delay support
- `Start/`
  Startup files and system initialization
- `Library/`
  STM32 standard peripheral library

## Core Logic

After power-on, the system initializes all hardware modules, reads the current clock, and enters the main loop.

In the main loop, the program continuously:

1. updates clock data
2. reads sensor data
3. processes Bluetooth commands
4. scans keys
5. checks reminder and alarm conditions
6. refreshes OLED display pages

## Main Functional Flow

- time is provided by `DS1302`
- medicine reminder is triggered according to configured schedule
- confirmation updates medicine state and stock
- `MAX30102` provides heart rate and blood oxygen data
- `DHT11` and `DS18B20` provide environmental and body temperature data
- abnormal values trigger alarm behavior
- OLED displays current data and medicine information by page

## Development Environment

- MCU: `STM32F103`
- IDE: `Keil uVision`
- language: `C`
- framework: STM32 Standard Peripheral Library

## Project Value

This project is suitable for:

- embedded systems learning
- STM32 peripheral integration practice
- graduation project reference
- health-monitoring device prototype development


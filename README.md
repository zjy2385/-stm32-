# 基于 STM32 的智能药盒系统设计

本项目基于 `STM32F103`，实现了药盒提醒、生命体征监测、本地显示和蓝牙串口交互。

## 功能

- `OLED` 多页面显示
- `MAX30102` 心率和血氧采集
- `DHT11` 环境温湿度采集
- `DS18B20` 体温采集
- `DS1302` 实时时钟
- 用药提醒与确认
- 蜂鸣器、LED、语音告警
- 蓝牙串口参数控制

## 硬件

- `STM32F103`
- `OLED`
- `MAX30102`
- `DHT11`
- `DS18B20`
- `DS1302`
- `JQ8400`
- 蜂鸣器
- LED
- 按键

## 目录

- `User/` 主程序和中断文件
- `Hardware/` 外设驱动和业务模块
- `System/` 延时等基础模块
- `Start/` 启动文件和系统文件
- `Library/` 标准库

## 说明

项目代码已按模块化方式整理，可用于学习和二次开发。

---

# Smart Pill Box System Based on STM32

This project is built on `STM32F103` and integrates medicine reminder, health monitoring, local display, and Bluetooth serial control.

## Features

- `OLED` multi-page display
- `MAX30102` heart rate and SpO2 acquisition
- `DHT11` ambient temperature and humidity acquisition
- `DS18B20` body temperature acquisition
- `DS1302` real-time clock
- medicine reminder and confirmation
- buzzer, LED, and voice alarm
- Bluetooth serial parameter control

## Hardware

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

## Structure

- `User/` main program and interrupt files
- `Hardware/` peripheral drivers and business modules
- `System/` basic delay support
- `Start/` startup and system files
- `Library/` STM32 standard peripheral library

## Notes

The code is organized in a modular way and can be used for learning and further development.


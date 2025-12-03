# STM32F103C6 UART Bootloader

A minimal UART bootloader for the STM32F103C6 microcontroller. This bootloader allows firmware updates over UART without using the BOOT0 pin, supports CRC verification, and provides basic commands for memory management and MCU information retrieval.

---

## Table of Contents

- [Features](#features)  
- [Hardware Requirements](#hardware-requirements)  
- [Software Requirements](#software-requirements)  
- [Bootloader Overview](#bootloader-overview)  
- [Supported Commands](#supported-commands)  
- [Memory Layout](#memory-layout)  
- [Usage](#usage)    

---

## Features

- UART communication for host MCU interaction  
- CRC32 verification for reliable firmware update  
- Jump to user application if it exists  
- Flash memory read/write and erase operations  
- Read protection management (RDP levels)  
- Supports ACK/NACK responses for host commands  
- Optional debug messages over UART  

---

## Hardware Requirements

- STM32F103C6 microcontroller  
- UART interface for host communication (e.g., USB-to-UART adapter)  
- Optional debug UART connection for logging  

---

## Software Requirements

- STM32CubeIDE or compatible toolchain  
- HAL (Hardware Abstraction Layer) libraries  
- CRC peripheral enabled for data integrity  
- UART peripherals configured for bootloader communication  

---

## Bootloader Overview

The bootloader executes on MCU reset. The main steps are:

1. Initialize HAL and system clocks  
2. Initialize peripherals (GPIO, CRC, UART)  
3. Check if user application exists at `FLASH_USER_APP_BASE_ADDRESS`  
4. If application exists, jump to it  
5. Otherwise, stay in bootloader mode and wait for host commands  

Key functions:

- `Check_APP_EXIST()` – Verifies if a valid user application is present  
- `Bootloader_Jump_To_User_App()` – Jumps to user application  
- `BL_UART_Fetch_Host_Command()` – Processes commands from host  

---

## Supported Commands

| Command | Description |
|---------|-------------|
| `0x10` | Get bootloader version |
| `0x11` | Get supported command list |
| `0x12` | Get chip identification number |
| `0x13` | Get Read Protection (RDP) status |
| `0x14` | Jump to specific address |
| `0x15` | Flash erase (page/mass) |
| `0x16` | Memory write |
| `0x17` | Enable/Disable write protection |
| `0x18` | Memory read |
| `0x19` | Read page protection status |
| `0x20` | Read OTP (One-Time Programmable) memory |
| `0x21` | Change read protection level |
| `0x22` | Exit bootloader (mark app as valid) |

---

## Memory Layout

| Region | Address Range | Description |
|--------|---------------|-------------|
| Bootloader | `0x08000000` – `0x080043FF` | Bootloader code |
| User Application | `0x08004400` – `0x08007BFF` | Firmware uploaded by bootloader |
| User App Flag | `0x08007C00` | Flag indicating user app existence |
| Flash End | `0x0800FFFF` | End of Flash memory |

---

## Usage

1. Compile the bootloader and flash it to STM32F103C6  
2. Connect the host MCU to the bootloader UART port  
3. On MCU reset, bootloader will check if user firmware exists  
4. If not, it waits for commands from the host  
5. Use commands to write firmware, erase memory, or jump to a specific address  

Example: Send **Get Bootloader Version** command from host MCU

```c
// Example: send command from STM32 host
uint8_t cmd[] = {0x05, 0x10, 0x00, 0x00, 0x00, 0x00}; // length + command + CRC
HAL_UART_Transmit(&huart2, cmd, sizeof(cmd), HAL_MAX_DELAY);

// Receive ACK and version response
uint8_t response[10];
HAL_UART_Receive(&huart2, response, sizeof(response), HAL_MAX_DELAY);

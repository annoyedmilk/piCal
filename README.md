# U_PiCalc_HS2023 - An AVR-based π Calculation System

This codebase is an implementation for AVR platforms that provides real-time approximation of the value of π (pi) using two different mathematical formulas: the Leibniz Series and the Nilkantha Method.

Created on: 3.10.2023  
Author: Annoyedmilk

## Features
- Calculation of π using two algorithms:
  - **Leibniz Series**
  - **Nilkantha Method**
- Ability to toggle between the algorithms via a button press.
- Display the current approximation of π, the method being used, and the elapsed time.
- Button-based user interface to start, stop, reset calculations, and switch between algorithms.

## File Description

### U_PiCalc_HS2023.c
The main codebase. It initializes system components, manages button inputs, and displays the results. Also, it contains implementations for various tasks to handle different functionalities of the system.

## Hardware Dependencies
- NHD0420Driver: For handling the display.
- ButtonHandler: For handling button inputs and debouncing.

## Software Dependencies
- FreeRTOS: Used for multitasking and managing various tasks such as button handling, display updating, and π calculations.

## Getting Started

1. **Setup the Hardware**: Ensure the AVR platform is correctly wired with the display (NHD0420Driver) and buttons (ButtonHandler).
2. **Compile and Upload**: Compile the `U_PiCalc_HS2023.c` file using an AVR-compatible C compiler and upload it to the AVR microcontroller.
3. **Interact**: After successful upload, use the buttons to start/stop the calculations, reset, or switch between algorithms.

## License
This project is open-source. Feel free to use, modify, and distribute it under the terms of the MIT license.

## Acknowledgements
Thanks to the AVR community for their extensive libraries and documentation.

## Issues and Contribution
For any issues, questions, or contributions, please open an issue on GitHub.
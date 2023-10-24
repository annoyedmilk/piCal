# AVR Pi Calculator (U_PiCalc_HS2023)

## Introduction

The AVR Pi Calculator, codenamed U_PiCalc_HS2023, is a sophisticated system designed to approximate the value of π (pi) using two renowned mathematical methods. Built on the AVR platform, this project showcases the power of embedded systems in mathematical computations.

**Created on**: 3.10.2023  
**Author**: Annoyedmilk

## Prerequisites

- **Hardware**:
  - AVR microcontroller board
  - NHD0420Driver compatible display
  - Four tactile buttons for input
- **Software**:
  - AVR-compatible C compiler
  - FreeRTOS library
  - AVR programming software

## Features

- **Dynamic Calculation**: Approximate π using:
  - **Leibniz Series**
  - **Nilkantha Method**
- **Interactive UI**: A button-driven interface allowing users to:
  - Start/Stop calculations
  - Reset computations
  - Toggle between approximation methods
- **Real-time Display**: View the current π approximation, the method in use, and the time elapsed since the start of the calculation.

## Installation

1. **Setup the Hardware**: Ensure the AVR platform is correctly wired with the display (NHD0420Driver) and buttons (ButtonHandler).
2. **Compile and Upload**: Compile the `main.c` file using an AVR-compatible C compiler and upload it to the AVR microcontroller.
3. **Interact**: After successful upload, use the buttons to start/stop the calculations, reset, or switch between algorithms.

## Experimental Branch

I have introduced an experimental branch that contains a refactored and optimized version of the Pi calculation tasks. This branch combines the two calculation methods into a single task, introduces clearer enumerations, and streamlines the button handling logic. It's a major overhaul aimed at improving code maintainability and potential performance. Feel free to check it out and provide feedback!

## File Description

1. **Clone the Repository**: Clone or download the project repository from GitHub.
2. **Setup the Hardware**:
   - Connect the AVR platform to the display using the NHD0420Driver.
   - Attach the four tactile buttons to the designated GPIO pins as described in the schematic (not provided here).
3. **Software Configuration**:
   - Ensure the AVR-compatible C compiler is set up correctly on your system.
   - Install the FreeRTOS library for AVR, if not already done.
4. **Compilation & Upload**:
   - Compile the `main.c` file.
   - Upload the compiled binary to the AVR microcontroller using your preferred programming tool.

## Usage

1. **Power On**: Start the AVR system.
2. **Select Method**: Use the designated button to choose between the Leibniz Series and the Nilkantha Method.
3. **Start Calculation**: Press the 'Start' button.
4. **View Results**: Monitor the display to see the real-time approximation of π, the method in use, and the elapsed time.
5. **Control**: Use the buttons to stop the calculations, reset them, or switch between methods as needed.

## FAQ

- **Q**: How accurate is the approximation?
  - **A**: The accuracy depends on the number of iterations and the method in use. Generally, the Nilkantha Method converges faster than the Leibniz Series.

- **Q**: Can I add more approximation methods?
  - **A**: Yes, the system is modular. You can integrate more methods by adding corresponding tasks and updating the control logic.

- **Q**: How do I troubleshoot display issues?
  - **A**: Ensure the NHD0420Driver is correctly connected and initialized in the code. Check the display's datasheet for specific troubleshooting steps.

## License

This software is open-source and is distributed under the MIT license.

## Acknowledgements

A heartfelt thank you to the AVR community for their invaluable libraries and documentation, which made this project possible.

## Issues and Contributions

Encounter a bug or have a feature request? Please open an issue on GitHub. Contributions to improve the project are always welcome!
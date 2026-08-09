# STM32 DC Motor Control & Monitoring System

*From theory to practice: turning datasheets, timers, and interrupt tables into a working closed-loop motor control system.*

## Overview

This project is a hands-on embedded systems build in which I designed and implemented a complete DC motor control and monitoring system on an STM32 microcontroller, programmed entirely at the register level (bare-metal, no HAL). The goal was not simply to make a motor spin, but to genuinely understand what happens beneath the abstraction layers that most tutorials hide: how a PWM signal is generated cycle by cycle, how an ADC converts a physical voltage into a digital temperature reading, how an encoder's electrical pulses become a measurable speed, and how interrupts let a microcontroller react to the real world in real time.

Building this project was where the theoretical concepts I had studied — timers, PWM, ADC/DAC, interrupt-driven design — stopped being lines in a textbook and became decisions I had to make, test, and debug on real hardware. That transition, from theoretical understanding to practical, working firmware, is the core value of this project.

## What I Learned

### 1. Reading and Using a Datasheet as a Primary Tool
Before this project, a datasheet was just reference material. Working at the register level forced me to treat it as the primary source of truth: locating the correct memory-mapped register addresses, understanding bit fields, respecting clock-enable sequencing, and cross-referencing the reference manual against the datasheet's electrical characteristics. This is one of the most transferable skills in embedded engineering — the ability to independently extract the information needed to configure any peripheral, on any microcontroller, without relying on pre-built abstractions.

### 2. PWM and Motor Control
I learned how a timer peripheral generates a PWM signal from first principles: how the prescaler (PSC) and auto-reload register (ARR) define the signal's frequency, and how the capture/compare register (CCRx) sets the duty cycle as a ratio of CCRx/ARR. Applying this to control a DC motor's speed made the relationship between timer configuration and physical motor behavior concrete rather than abstract.

- PWM frequency: `TIM_CLK / (PSC + 1) / (ARR + 1)`
- Duty cycle: `CCRx / ARR`

### 3. ADC and the ADC/DAC Concept
Measuring motor temperature with a TMP36 sensor required configuring the ADC from scratch: selecting the correct clock source in the RCC, enabling the ADC's power control clock, configuring sampling time, and converting the raw digital reading into a real-world temperature value using the sensor's transfer equation. This gave me a practical understanding of the ADC/DAC conversion concept — how continuous analog voltages are sampled and quantized into digital values (and conceptually, the reverse process a DAC performs) — rather than treating "analog input" as a black box function call.

### 4. Interrupt-Driven Design
The entire application is interrupt-driven rather than polling in a busy loop. I learned to design around Interrupt Service Routines (ISRs): keeping them short and non-blocking, using flags to defer time-intensive work (such as redrawing the display) to the main loop, and correctly clearing interrupt pending flags at the register level. This taught me why interrupt design is as much about *what not to do inside an ISR* as it is about handling the event itself.

### 5. Timer-Based Signal Measurement
Using a timer in encoder mode (and later a custom state machine) to measure motor speed from a quadrature encoder taught me how timers are far more than PWM generators — they are versatile peripherals capable of capturing, counting, and decoding real-world signals.

## Why CMSIS?

This project relies on **CMSIS** (Cortex Microcontroller Software Interface Standard), the vendor-provided header and access layer for the ARM Cortex-M core and peripherals.

**Benefits of using CMSIS:**
- Provides standardized, human-readable names for memory-mapped registers (e.g. `TIM4->SR`, `GPIOA->MODER`) instead of raw hexadecimal addresses.
- Supplies core access functions for the NVIC, SysTick, and other Cortex-M core peripherals, so interrupt configuration doesn't require manually calculating core register offsets.
- Improves code portability and readability across different STM32 families, since the naming conventions are consistent.
- Reduces the risk of address/bit-mask errors compared to writing everything by hand.

**Without CMSIS**, every peripheral register would need to be manually defined as a raw memory address (using pointer casting to fixed addresses from the reference manual), and every bit mask would need to be defined and maintained independently. This is possible, but it removes an entire layer of readability and safety, and makes the code significantly more error-prone and harder to port to another microcontroller. In practice, not using CMSIS means writing and maintaining your own miniature version of it — with none of the vendor validation.

## Why u8g2_csrc?

The display subsystem uses the **u8g2** library ([https://github.com/olikraus/u8g2](https://github.com/olikraus/u8g2)) to drive the SPI OLED display (SSD1306 controller).

u8g2 is essential here because it abstracts away two very complex subsystems that would otherwise have to be built from scratch:
- **Display driver logic**: initialization sequences, page/column addressing, and pixel buffer management specific to the SSD1306 controller.
- **Font rendering and drawing primitives**: text rendering, shapes, and layout, which involve non-trivial bitmap/font-encoding logic.

By integrating u8g2_csrc and implementing only a thin hardware interface layer (the SPI driver functions this project provides), the application layer can focus on *what* to display — motor speed and temperature — instead of *how* to physically address individual pixels. This is a good illustration of proper module encapsulation: the hardware application layer (`display.c`) stays clean and focused, while a well-tested third-party library handles the low-level complexity.

## Hardware Architecture

### Components
- STM32 Nucleo development board (see board discussion below)
- KY-040 rotary encoder
- 12V DC motor with integrated encoder
- L298N motor driver board and 12V DC power supply
- SPI OLED display, 128x64 resolution (SSD1306 controller)
- TMP36 analog temperature sensor

### Hardware Connections

| Nucleo Board Pin | MCU Signal | Hardware Interface |
|---|---|---|
| PC10 | SPI3_SCK | Display D0 |
| PC11 | Output | Display CS |
| PC12 | SPI3_MOSI | Display D1 |
| PD2 | Output | Display DC |
| PA15 | Output | Display RES |
| PA11 | Timer Input | Motor Encoder A |
| PA12 | Timer Input | Motor Encoder B |
| PC7 | Output | Motor Driver In1 |
| PA9 | Output | Motor Driver In2 |
| PB6 | Timer Output | Motor Driver PWM |
| PB13 | Input | Rotary Encoder CLK |
| PB14 | Input | Rotary Encoder DT |
| PB15 | Input (Interrupt) | Rotary Encoder SW |
| PA1 | ADC Input | Temp Sensor Output |

### Software Design

The firmware is split into two clear layers, which made the codebase easier to reason about and to potentially port to another microcontroller:

**Peripheral configuration layer** (MCU-specific):
- `adc.c` — configures the ADC
- `clockconfig.c` — configures the system clock (PLL) and enables peripheral clocks
- `gpio.c` — configures all GPIO modes and parameters
- `interrupt.c` — contains all Interrupt Service Routines
- `spi.c` — implements the SPI driver (start transfer, send byte, end transfer)
- `timer.c` — configures all timers

**Hardware application layer** (largely MCU-agnostic):
- `display.c` — calls u8g2 functions to draw the display, pulling data from other modules
- `encoder.c` — implements the rotary encoder driver
- `motor.c` — implements direction control, duty cycle setting, speed measurement, and a rolling average of speed; encapsulates the motor's status data and exposes interface functions
- `temperature.c` — takes and converts ADC measurements into a temperature value, and exposes it via an interface function

## Board Choice: Why This Nucleo Board (and Portability to the STM32F446RE)

This project uses an **STM32 Nucleo-G431RB** (STM32G4 series). The G4 series is deliberately positioned by ST as a mixed-signal, motor-control-oriented family: it offers enhanced analog peripherals (fast, high-resolution ADCs, integrated DACs, comparators) and advanced timers well suited to precise PWM generation and signal capture — exactly the kind of peripheral set a motor control and sensor-monitoring project benefits from.

**Can this be built on an STM32F446RE (Nucleo-F446RE) instead?**
Yes — the project is portable to the **STM32F446RE**, which is also a Cortex-M4 core with a comparable peripheral set (GPIO, general-purpose timers, ADC, SPI, EXTI). However, because this firmware is written at the register level rather than through HAL, the port is **not just a recompile**. The following would need to change:

- **CMSIS device header**: the CMSIS header must correspond to the F446 device (register maps, base addresses, and bit definitions differ between the G4 and F4 families).
- **Clock configuration (`clockconfig.c`)**: the RCC/PLL configuration must be rewritten for the F446's clock tree and maximum operating frequency (180 MHz vs. 170 MHz), including flash latency wait states.
- **Timer instance mapping**: pin-to-timer alternate function mappings differ between the two families, so PWM output, encoder input, and timer-interrupt pins must be reassigned according to the F446's alternate function tables.
- **ADC configuration (`adc.c`)**: while the general ADC configuration approach is similar, register names and some configuration steps differ, since the F4 series ADC block is a different IP version than the G4's.
- **GPIO pin assignments**: the pinout table above would need to be re-mapped to available and electrically appropriate pins on the F446RE's Nucleo footprint.

In short: the **application layer** (`display.c`, `encoder.c`, `motor.c`, `temperature.c`, and the u8g2 integration) is largely reusable as-is, since it operates on abstracted data and interfaces. The **peripheral configuration layer** would need a targeted rewrite against the STM32F446RE's reference manual and CMSIS headers — which is, in fact, a valuable exercise in itself, since it reinforces the datasheet-driven, register-level skill this project was built to develop.

## u8g2 Library
This project integrates the [u8g2 library](https://github.com/olikraus/u8g2) for OLED display rendering, interfaced through a custom SPI driver written for this project.

## Challenges, Learnings, and Notes

- The GPIO peripheral clock must be enabled before its configuration registers can be written.
- Clearing the GPIOA MODER register disabled JTAG — several GPIOA pins are used by the JTAG debug interface and had to be left at their reset configuration.
- Configuring the system clock required setting flash latency to 4 wait states, per the reference manual and ST example code.
- Standard console output (printf) required a custom low-level write function using CMSIS drivers, since it isn't functional out of the box.
- Interrupt pending flags are best cleared by writing the bit directly rather than combining it with `|` or `&` against the current register value, since these flag bits are cleared by writing 0 or 1 (register-dependent) and unrelated bits are unaffected regardless.
- The rotary encoder proved electrically noisy; a debounced state machine (tracking movement and stop states) proved more reliable than relying solely on a timer in encoder mode.
- Polling the encoder at 30 Hz gave reliable results; an initial 5 Hz polling rate caused missed pulses and incorrect direction detection.
- SPI hardware NSS (automatic chip-select) behavior was unintuitive; a manually toggled GPIO was used as chip select instead, for clarity and reliability.
- Display redraws are too time-intensive to run safely inside an ISR; a flag is set in the ISR and the redraw is executed from the main loop instead.
- ADC configuration requires enabling both the correct clock source in the RCC and the ADC's dedicated power control clock — a step that is easy to miss.

## Future Improvements

- Improve motor startup behavior from zero speed (e.g. a brief 100% duty "kick-start" phase).
- Validate temperature measurement accuracy against a reference sensor.
- Extend parameter validation and error handling across all modules.
- Move the ADC measurement from a blocking method to an interrupt-driven or DMA-based approach.
- Move the SPI transfer from a blocking method to an interrupt-driven or DMA-based approach.
- Provide a hardware-level fix for the display RES line noise sensitivity (e.g. a pull-up or tying it to a steady supply voltage) in addition to the current software-controlled approach.

## Acknowledgements

- u8g2 display library: [https://github.com/olikraus/u8g2](https://github.com/olikraus/u8g2)
- Original project concept and reference implementation: [ncaccamo/STM32_DCMotor_Demonstration](https://github.com/ncaccamo/STM32_DCMotor_Demonstration)

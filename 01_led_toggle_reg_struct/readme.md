# STM32F446RE — Bare-Metal LED Toggle (Register-Level Programming)

A minimal embedded systems project demonstrating **direct register manipulation** on an STM32F446RE (Nucleo-64) microcontroller — no HAL, no CubeMX-generated abstraction layers. Every peripheral is configured by writing straight to the memory-mapped registers described in the reference manual (RM0390).

## Why This Project

Most STM32 tutorials start and end with STM32CubeMX and the HAL library. This project intentionally goes one layer deeper: it builds the GPIO and RCC peripheral structures from scratch using `volatile` pointer casts and raw memory offsets, the same way the HAL itself is implemented under the hood.

The goal is to demonstrate a solid understanding of:
- How the ARM Cortex-M memory map is organized (peripheral base addresses, bus offsets)
- How peripheral registers are structured and accessed via `struct` overlays
- Clock gating and the role of `RCC->AHB1ENR` in enabling peripherals
- GPIO configuration at the bit level: mode selection, output type, and atomic set/reset via `BSRR`
- Why this knowledge matters even when using HAL/CubeMX day-to-day — it's what lets you debug issues HAL abstracts away, optimize critical paths, and work confidently on custom boards without generated code

## What It Does

Toggles the onboard user LED (**LD2**, connected to **PA5**) on the Nucleo board in an infinite loop, with a simple busy-wait delay between transitions.

## Technical Highlights

| Concept | Implementation |
|---|---|
| Peripheral addressing | Base addresses computed manually from `PERIPH_BASE` + bus offsets (`AHB1PERIPH_OFFSET`, `GPIOA_OFFSET`, `RCC_OFFSET`) |
| Register access | `GPIO_TypeDef` and `RCC_TypeDef` structs mapped onto memory with `volatile` pointers, matching the exact register layout from the datasheet |
| Clock enable | `RCC->AHB1ENR` bit set to power up the GPIOA peripheral before configuration |
| Pin configuration | `MODER` register cleared and set to configure PA5 as general-purpose output |
| Atomic pin control | `BSRR` register used for glitch-free set/reset of the output pin (upper 16 bits reset, lower 16 bits set) |

## Hardware

- **Board:** NUCLEO-F446RE
- **MCU:** STM32F446RET6 (ARM Cortex-M4)
- **LED:** LD2, GPIO Port A, Pin 5

## Toolchain

- STM32CubeIDE
- GNU ARM Embedded Toolchain (arm-none-eabi-gcc)
- ST-Link for flashing/debugging

## Build & Flash

1. Open the project in STM32CubeIDE (`.cproject` / `.project` already included).
2. Build the project (`Ctrl+B`).
3. Connect the Nucleo board via USB.
4. Flash and run using the provided debug launch configuration.

## About This Series

This is part of a broader effort to master STM32 development from the register level up before relying on HAL/CubeMX abstractions — building the kind of low-level intuition that makes higher-level tools easier to use effectively and easier to debug when they fail.

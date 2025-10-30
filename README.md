# STM32 ARM Assembly Interpreter

An interactive ARM assembly language interpreter running on STM32 microcontroller (Cortex-M4). Execute ARM Thumb-2 instructions directly on hardware via UART terminal.

## Quick Start
<!--
### Hardware
- STM32 Nucleo-L476RG board
- USB cable (ST-Link + UART)
-->

### Software
- STM32CubeIDE 1.19.0+
- Serial terminal (115200 baud, 8N1)

### Build & Flash
```bash
# Using STM32CubeIDE: Import project → Build (Ctrl+B) → Run
# Or command line:
cd Debug && make -j4 all
STM32_Programmer_CLI -c port=SWD -w stm32_asm_interpreter.elf -rst
```

> [!IMPORTANT]  
> The project is in its early stages of development, and the scripts for generating code for different STM32 microcontrollers are not yet ready. Currently, the project needs to be manually adapted to the different STM32 microcontrollers.

## Documentation

For detailed documentation, see the Github Wiki (coming soon):
- Architecture overview
- Adding new instructions  
- Parser internals
- Error handling guide
- Testing procedures

## Contributing

You are more than welcome to contribute to the project! Fork → Branch → PR.

## Acknowledgments

STM32 HAL • ARM Cortex-M4 Documentation • STM32CubeIDE

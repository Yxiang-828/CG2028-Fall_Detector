# STM32L4+ GPIO Register Summary

This document distills the register information from the STM32L4+ reference manual (RM0432, section 8.4) into an easy-to-read cheat sheet. Only the essential fields and descriptions are retained; redundant or peripheral material has been removed.

## Port register overview
Each GPIO port (A through I) has the following 32-bit registers at its base address (e.g. `GPIOB_BASE = 0x4800_0400`):

| Register | Full name | Purpose |
|----------|-----------|---------|
| **MODER** | Mode register | Select pin mode: 00=analog, 01=general-purpose output, 10=input, 11=alternate function. Two bits per pin. |
| **OTYPER** | Output type register | Output type when in output mode: 0=push-pull, 1=open-drain. One bit per pin. |
| **OSPEEDR** | Output speed register | Drive strength / speed for outputs (low/med/high/very-high). Two bits per pin. |
| **PUPDR** | Pull-up/pull-down register | Internal resistor: 00=none, 01=pull-up, 10=pull-down. Two bits per pin. |
| **IDR** | Input data register | Read-only: current level of pins. One bit each. |
| **ODR** | Output data register | Read/write output latch. Takes effect only if pin is in output mode. |
| **BSRR** | Bit set/reset register | Atomic set/reset of outputs; lower half sets, upper half resets. Useful for toggling without read-modify-write. |
| **LCKR** | Configuration lock register | Lock configuration of selected pins until next reset. Prevents accidental modification. |
| **AFRL** | Alternate function low register | Select alternate function (AF0–AF15) for pins 0–7. Four bits per pin. |
| **AFRH** | Alternate function high register | Same as AFRL but for pins 8–15. |
| **BRR** | Bit reset register | Write 1 to reset corresponding output bit (simpler form of BSRR). |

> **Note:** `GPIOx_MODER` is located at offset `0x00` from the port base; other registers follow in the order above with increments of 0x04.

## Practical usage notes
- After reset all pins default to **analog mode** (MODER bits = 00). If you intend to use a pin as GPIO, you must write the appropriate bits in `MODER` first.
- To change only one pin without disturbing the others, perform a read-modify-write: 
  ```asm
  LDR  R1, =GPIOB_MODER
  LDR  R0, [R1]
  BIC  R0, R0, #(3 << (2*14))   ; clear bits for PB14
  ORR  R0, R0, #(1 << (2*14))   ; set mode=01 (output)
  STR  R0, [R1]
  ```
- Use `BSRR`/`BRR` to update outputs atomically (no need to read `ODR`).
- Configure `OTYPER`, `OSPEEDR`, and `PUPDR` immediately after `MODER` if specific output type/speed/pull is required.
- Lock configuration with `LCKR` only when you want to prevent any further software writes (e.g. critical pins).

This file is intended as a quick reference for assembly or C code development; consult the full RM0432 for complete bit-field tables and reset values.

# ARMv7-M Thumb Instruction Set - Consolidated Reference

## Table of Contents
1. [Branch Instructions](#branch-instructions)
2. [Data Processing Instructions](#data-processing-instructions)
3. [Load and Store Instructions](#load-and-store-instructions)
4. [Multiply, Divide & DSP Instructions](#multiply-divide--dsp-instructions)
5. [Bit Field & Shift Instructions](#bit-field--shift-instructions)
6. [Compare & Test Instructions](#compare--test-instructions)
7. [System & Barrier Instructions](#system--barrier-instructions)
8. [Conditional Execution](#conditional-execution)
9. [Floating-Point Instructions (FP Extension)](#floating-point-instructions)
10. [Standard Assembler Syntax](#standard-assembler-syntax)

---

## Branch Instructions

| Instruction | Syntax | Description | Range |
|-------------|--------|-------------|-------|
| **B** | `B <label>` | Branch to target address | ±16 MB (T4 encoding) |
| **B** (cond) | `B<c> <label>` | Conditional branch | ±1 MB (T3) / -256 to +254 bytes (T1) |
| **BL** | `BL<c> <label>` | Branch with Link (subroutine call) | ±16 MB |
| **BX** | `BX<c> <Rm>` | Branch and Exchange (register) | Any address |
| **BLX** | `BLX<c> <Rm>` | Branch with Link and Exchange | Any address |
| **CBZ/CBNZ** | `CBZ <Rn>, <label>` / `CBNZ <Rn>, <label>` | Compare and Branch on Zero/Non-Zero | 0 to 126 bytes (forward only) |
| **TBB/TBH** | `TBB [<Rn>, <Rm>]` / `TBH [<Rn>, <Rm>, LSL #1]` | Table Branch (byte/halfword offsets) | 0-510B / 0-131070B |

### CBZ/CBNZ Details
```
CB{N}Z<q> <Rn>, <label>
```
- **Operation**: Compares register value with zero, branches if zero (CBZ) or non-zero (CBNZ)
- **Offset Range**: Even numbers from 0 to 126 bytes (forward branches only)
- **Not permitted in IT block**
- **Condition flags**: Not affected

---

## Data Processing Instructions

### Arithmetic Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **ADD** | `ADD{S} <Rd>, <Rn>, #<const>` / `ADD{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Add (immediate or register) |
| **ADC** | `ADC{S} <Rd>, <Rn>, #<const>` / `ADC{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Add with Carry |
| **SUB** | `SUB{S} <Rd>, <Rn>, #<const>` / `SUB{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Subtract |
| **SBC** | `SBC{S} <Rd>, <Rn>, #<const>` / `SBC{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Subtract with Carry |
| **RSB** | `RSB{S} <Rd>, <Rn>, #<const>` / `RSB{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Reverse Subtract |
| **ADR** | `ADR <Rd>, <label>` | Load PC-relative address |

### Logical Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **AND** | `AND{S} <Rd>, <Rn>, #<const>` / `AND{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Bitwise AND |
| **ORR** | `ORR{S} <Rd>, <Rn>, #<const>` / `ORR{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Bitwise OR |
| **EOR** | `EOR{S} <Rd>, <Rn>, #<const>` / `EOR{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Bitwise XOR (Exclusive OR) |
| **BIC** | `BIC{S} <Rd>, <Rn>, #<const>` / `BIC{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Bit Clear (AND with NOT) |
| **ORN** | `ORN{S} <Rd>, <Rn>, #<const>` / `ORN{S} <Rd>, <Rn>, <Rm>{,<shift>}` | Bitwise OR NOT |
| **MVN** | `MVN{S} <Rd>, #<const>` / `MVN{S} <Rd>, <Rm>{,<shift>}` | Move NOT (bitwise complement) |
| **MOV** | `MOV{S} <Rd>, #<const>` / `MOV{S} <Rd>, <Rm>{,<shift>}` | Move (copy value) |

### Move Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **MOV** | `MOV{S} <Rd>, #<imm8>` / `MOV{S} <Rd>, #<const>` / `MOV{S} <Rd>, <Rm>{,<shift>}` | Move immediate or register |
| **MOVT** | `MOVT <Rd>, #<imm16>` | Move Top (writes top 16 bits) |
| **MVN** | `MVN{S} <Rd>, #<const>` / `MVN{S} <Rd>, <Rm>{,<shift>}` | Move NOT |

---

## Load and Store Instructions

### Single Register Load/Store

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **LDR** | `LDR <Rt>, [<Rn>, #<imm>]` / `LDR <Rt>, [<Rn>, <Rm>]` | Load Word |
| **LDRB** | `LDRB <Rt>, [<Rn>, #<imm>]` / `LDRB <Rt>, [<Rn>, <Rm>]` | Load Byte (zero-extend) |
| **LDRH** | `LDRH <Rt>, [<Rn>, #<imm>]` / `LDRH <Rt>, [<Rn>, <Rm>]` | Load Halfword (zero-extend) |
| **LDRSB** | `LDRSB <Rt>, [<Rn>, #<imm>]` / `LDRSB <Rt>, [<Rn>, <Rm>]` | Load Signed Byte (sign-extend) |
| **LDRSH** | `LDRSH <Rt>, [<Rn>, #<imm>]` / `LDRSH <Rt>, [<Rn>, <Rm>]` | Load Signed Halfword (sign-extend) |
| **STR** | `STR <Rt>, [<Rn>, #<imm>]` / `STR <Rt>, [<Rn>, <Rm>]` | Store Word |
| **STRB** | `STRB <Rt>, [<Rn>, #<imm>]` / `STRB <Rt>, [<Rn>, <Rm>]` | Store Byte |
| **STRH** | `STRH <Rt>, [<Rn>, #<imm>]` / `STRH <Rt>, [<Rn>, <Rm>]` | Store Halfword |

### Load/Store Doubleword

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **LDRD** | `LDRD <Rt>, <Rt2>, [<Rn>, #<imm>]` | Load Two Words |
| **STRD** | `STRD <Rt>, <Rt2>, [<Rn>, #<imm>]` | Store Two Words |

### Load/Store Multiple

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **LDM** | `LDM <Rn>{!}, <registers>` | Load Multiple (Increment After) |
| **LDMDB** | `LDMDB <Rn>{!}, <registers>` | Load Multiple (Decrement Before) |
| **STM** | `STM <Rn>{!}, <registers>` | Store Multiple (Increment After) |
| **STMDB** | `STMDB <Rn>{!}, <registers>` | Store Multiple (Decrement Before) |
| **PUSH** | `PUSH <registers>` | Push registers to stack (equivalent to STMDB SP!, ...) |
| **POP** | `POP <registers>` | Pop registers from stack (equivalent to LDM SP!, ...) |

### Exclusive Access (for synchronization)

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **LDREX** | `LDREX <Rt>, [<Rn>]` | Load Exclusive Word |
| **LDREXB** | `LDREXB <Rt>, [<Rn>]` | Load Exclusive Byte |
| **LDREXH** | `LDREXH <Rt>, [<Rn>]` | Load Exclusive Halfword |
| **STREX** | `STREX <Rd>, <Rt>, [<Rn>]` | Store Exclusive Word |
| **STREXB** | `STREXB <Rd>, <Rt>, [<Rn>]` | Store Exclusive Byte |
| **STREXH** | `STREXH <Rd>, <Rt>, [<Rn>]` | Store Exclusive Halfword |
| **CLREX** | `CLREX` | Clear Exclusive (clear exclusive monitor) |

### Addressing Modes

| Mode | Syntax | Description |
|------|--------|-------------|
| Offset | `[<Rn>, <offset>]` | Add offset to base, base unchanged |
| Pre-indexed | `[<Rn>, <offset>]!` | Add offset to base, write back to base |
| Post-indexed | `[<Rn>], <offset>` | Use base address, then add offset and write back |

---

## Multiply, Divide & DSP Instructions

### Basic Multiply

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **MUL** | `MUL <Rd>, <Rn>, <Rm>` | Multiply (32-bit result) |
| **MLA** | `MLA <Rd>, <Rn>, <Rm>, <Ra>` | Multiply Accumulate: Rd = Ra + (Rn × Rm) |
| **MLS** | `MLS <Rd>, <Rn>, <Rm>, <Ra>` | Multiply Subtract: Rd = Ra - (Rn × Rm) |

### Long Multiply (64-bit result)

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **SMULL** | `SMULL <RdLo>, <RdHi>, <Rn>, <Rm>` | Signed Multiply Long |
| **UMULL** | `UMULL <RdLo>, <RdHi>, <Rn>, <Rm>` | Unsigned Multiply Long |
| **SMLAL** | `SMLAL <RdLo>, <RdHi>, <Rn>, <Rm>` | Signed Multiply Accumulate Long |
| **UMLAL** | `UMLAL <RdLo>, <RdHi>, <Rn>, <Rm>` | Unsigned Multiply Accumulate Long |

### Hardware Divide (ARMv7-M)

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **SDIV** | `SDIV <Rd>, <Rn>, <Rm>` | Signed Divide |
| **UDIV** | `UDIV <Rd>, <Rn>, <Rm>` | Unsigned Divide |

**Note**: Divide-by-zero behavior controlled by CCR.DIV_0_TRP bit:
- `DIV_0_TRP == 0`: Returns 0 on divide-by-zero
- `DIV_0_TRP == 1`: Generates UsageFault exception on divide-by-zero

---

## Bit Field & Shift Instructions

### Shift Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **LSL** | `LSL{S} <Rd>, <Rm>, #<imm>` / `LSL{S} <Rd>, <Rn>, <Rm>` | Logical Shift Left |
| **LSR** | `LSR{S} <Rd>, <Rm>, #<imm>` / `LSR{S} <Rd>, <Rn>, <Rm>` | Logical Shift Right |
| **ASR** | `ASR{S} <Rd>, <Rm>, #<imm>` / `ASR{S} <Rd>, <Rn>, <Rm>` | Arithmetic Shift Right |
| **ROR** | `ROR{S} <Rd>, <Rm>, #<imm>` / `ROR{S} <Rd>, <Rn>, <Rm>` | Rotate Right |
| **RRX** | `RRX{S} <Rd>, <Rm>` | Rotate Right with Extend |

### Bit Field Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **BFC** | `BFC <Rd>, #<lsb>, #<width>` | Bit Field Clear (clears adjacent bits) |
| **BFI** | `BFI <Rd>, <Rn>, #<lsb>, #<width>` | Bit Field Insert (copies bits from Rn to Rd) |
| **SBFX** | `SBFX <Rd>, <Rn>, #<lsb>, #<width>` | Signed Bit Field Extract |
| **UBFX** | `UBFX <Rd>, <Rn>, #<lsb>, #<width>` | Unsigned Bit Field Extract |

### Byte Reverse Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **REV** | `REV <Rd>, <Rm>` | Byte-Reverse Word (swap byte order) |
| **REV16** | `REV16 <Rd>, <Rm>` | Byte-Reverse Packed Halfword |
| **REVSH** | `REVSH <Rd>, <Rm>` | Byte-Reverse Signed Halfword |
| **RBIT** | `RBIT <Rd>, <Rm>` | Reverse Bits (bitwise reversal) |

---

## Compare & Test Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **CMP** | `CMP <Rn>, #<const>` / `CMP <Rn>, <Rm>{,<shift>}` | Compare (subtract, discard result, set flags) |
| **CMN** | `CMN <Rn>, #<const>` / `CMN <Rn>, <Rm>{,<shift>}` | Compare Negative (add, discard result, set flags) |
| **TST** | `TST <Rn>, #<const>` / `TST <Rn>, <Rm>{,<shift>}` | Test (AND, discard result, set flags) |
| **TEQ** | `TEQ <Rn>, #<const>` / `TEQ <Rn>, <Rm>{,<shift>}` | Test Equivalence (XOR, discard result, set flags) |

### How Condition Flags (N, Z, C, V) are Affected
These instructions do not save the mathematical result; their sole purpose is to update the Condition Code Flags in the APSR (Application Program Status Register).

| Instruction | Math Operation | N (Negative) | Z (Zero) | C (Carry) | V (Overflow) |
|-------------|----------------|--------------|----------|-----------|--------------|
| **CMP** | `Rn - Op2` | `1` if result < 0 | `1` if result == 0 (`Rn == Op2`) | `1` if **NO borrow** (`Rn >= Op2` unsigned) | `1` if signed overflow occurred |
| **CMN** | `Rn + Op2` | `1` if result < 0 | `1` if result == 0 (`Rn == -Op2`) | `1` if unsigned carry out | `1` if signed overflow occurred |
| **TST** | `Rn AND Op2` | `1` if MSB (bit 31) is `1` | `1` if result == 0 (tested bits are `0`) | Updates based on barrel shifter | Unaffected |
| **TEQ** | `Rn EOR Op2` | `1` if MSB (bit 31) is `1` | `1` if result == 0 (`Rn == Op2`) | Updates based on barrel shifter | Unaffected |

**Key Flag Nuances**:
- **`TST` / `TEQ` limits**: Since these are logical operations, they naturally cannot overflow, so `V` is left unchanged. The `C` flag is updated by the carry-out of the barrel shifter evaluating the second operand. If no barrel shift is used, `C` remains unaffected.
- **`CMP` unsigned meaning**: Because ARM handles subtraction by adding the inverted Operand2 with a carry-in, a `C=1` flag actually means a borrow did *not* occur. Functionally, `C=1` means `Rn >= Op2` (Unsigned Higher or Same). `C=0` means `Rn < Op2` (Unsigned Lower).

### Practical Uses for TST & TEQ
While `CMP` is used to see which number is larger or smaller, `TST` and `TEQ` are used for **bit manipulation and checking**.

#### TST (Test via Bitwise AND)
Used to check if a specific bit (or combination of bits) is `1` or `0`, without altering the original register. 
- **Example:** You want to check if bit 3 (the 4th bit from the right, value `8`) in `R0` is a `1`.
  - `TST R0, #8` computes `R0 AND 0b1000`.
  - If bit 3 was `0`, the result is `0`. The Z (Zero) flag is set to `1` (Meaning: Yes, it is zero).
  - If bit 3 was `1`, the result is `8`. The Z (Zero) flag is set to `0` (Meaning: No, it is not zero).
  - You can then use `BNE` (Branch Not Equal / Not Zero) to execute code if the bit was `1`.

#### TEQ (Test Equivalence via Bitwise XOR)
Used to check if two values have the same sign, or to see if two registers are perfectly equal without risking an arithmetic overflow like `CMP` might.
- **Example:** Checking if two numbers have the same parity/sign.
  - `TEQ R0, R1` computes `R0 XOR R1`.
  - If they are completely identical, `XOR` results in `0`, so the Z (Zero) flag is set to `1`.
  - If `R0` is Negative and `R1` is Positive, their MSBs (Bit 31) are `1` and `0`. `1 XOR 0 = 1`. The result's MSB is `1`, so the N (Negative) flag is set to `1`. You can then use `BMI` (Branch if Minus) to run code if their signs differ!

---

## System & Barrier Instructions

### System Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **SVC** | `SVC #<imm8>` | Supervisor Call (system call) |
| **BKPT** | `BKPT #<imm8>` | Breakpoint (debug) |
| **MRS** | `MRS <Rd>, <spec_reg>` | Move from Special Register to ARM register |
| **MSR** | `MSR <spec_reg>, <Rn>` | Move to Special Register from ARM register |
| **CPS** | `CPS<effect> <flags>` | Change Processor State |

### Barrier Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **DMB** | `DMB {<option>}` | Data Memory Barrier (ensures memory access ordering) |
| **DSB** | `DSB {<option>}` | Data Synchronization Barrier (complete memory accesses) |
| **ISB** | `ISB {<option>}` | Instruction Synchronization Barrier (flush pipeline) |

### Hint Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **NOP** | `NOP` | No Operation |
| **WFE** | `WFE` | Wait For Event (low-power wait) |
| **WFI** | `WFI` | Wait For Interrupt (low-power wait) |
| **SEV** | `SEV` | Send Event (wake other cores) |
| **YIELD** | `YIELD` | Yield (hint for thread switching) |
| **DBG** | `DBG #<imm4>` | Debug hint |

### Preload Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **PLD** | `PLD [<Rn>, #<imm>]` / `PLD [<Rn>, <Rm>]` | Preload Data (cache hint) |
| **PLI** | `PLI [<Rn>, #<imm>]` / `PLI [<Rn>, <Rm>]` | Preload Instruction (cache hint) |

---

## Conditional Execution

### IT Instruction (If-Then)

The IT instruction makes up to 4 following instructions conditional:

```
IT{x{y{z}}} <cond>
```

- **x, y, z**: Each can be `T` (Then) or `E` (Else)
- **cond**: Base condition code

**Examples**:
```asm
ITTE EQ          ; Next 3 instructions: EQ, EQ, NE
IT GT            ; Next instruction only: GT
ITETT VS         ; Next 4 instructions: VS, VC, VS, VS
```

**Rules**:
- Instructions in IT block must match or be inverse of base condition
- Branch instructions can only be last instruction in IT block
- Cannot nest IT blocks

### Condition Codes

| Code | Mnemonic | Meaning (Integer) | Flags |
|------|----------|-------------------|-------|
| 0000 | EQ | Equal | Z == 1 |
| 0001 | NE | Not Equal | Z == 0 |
| 0010 | CS/HS | Carry Set / Unsigned Higher or Same | C == 1 |
| 0011 | CC/LO | Carry Clear / Unsigned Lower | C == 0 |
| 0100 | MI | Minus / Negative | N == 1 |
| 0101 | PL | Plus / Positive or Zero | N == 0 |
| 0110 | VS | Overflow | V == 1 |
| 0111 | VC | No Overflow | V == 0 |
| 1000 | HI | Unsigned Higher | C == 1 && Z == 0 |
| 1001 | LS | Unsigned Lower or Same | C == 0 \|\| Z == 1 |
| 1010 | GE | Signed Greater or Equal | N == V |
| 1011 | LT | Signed Less Than | N != V |
| 1100 | GT | Signed Greater Than | Z == 0 && N == V |
| 1101 | LE | Signed Less or Equal | Z == 1 \|\| N != V |
| 1110 | AL | Always (unconditional) | Any |

---

## Floating-Point Instructions

> **Note**: Requires FP extension (FPv4-SP or FPv5)

### FP Data Processing

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **VADD** | `VADD.F32 <Sd>, <Sn>, <Sm>` | FP Add |
| **VSUB** | `VSUB.F32 <Sd>, <Sn>, <Sm>` | FP Subtract |
| **VMUL** | `VMUL.F32 <Sd>, <Sn>, <Sm>` | FP Multiply |
| **VDIV** | `VDIV.F32 <Sd>, <Sn>, <Sm>` | FP Divide |
| **VABS** | `VABS.F32 <Sd>, <Sm>` | FP Absolute Value |
| **VNEG** | `VNEG.F32 <Sd>, <Sm>` | FP Negate |
| **VSQRT** | `VSQRT.F32 <Sd>, <Sm>` | FP Square Root |
| **VMLA** | `VMLA.F32 <Sd>, <Sn>, <Sm>` | FP Multiply Accumulate |
| **VMLS** | `VMLS.F32 <Sd>, <Sn>, <Sm>` | FP Multiply Subtract |
| **VMOV** | `VMOV.F32 <Sd>, #<imm>` / `VMOV <Sd>, <Sm>` | FP Move |
| **VCMP** | `VCMP.F32 <Sd>, <Sm>` / `VCMP.F32 <Sd>, #0` | FP Compare |

### FP Convert

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **VCVT** | `VCVT.F32.S32 <Sd>, <Sm>` / `VCVT.S32.F32 <Sd>, <Sm>` | Convert between FP and integer |
| **VCVT** | `VCVT.F64.F32 <Dd>, <Sm>` / `VCVT.F32.F64 <Sd>, <Dm>` | Convert between single and double precision |

### FP Load/Store

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **VLDR** | `VLDR <Sd>, [<Rn>, #<imm>]` | FP Load Register |
| **VSTR** | `VSTR <Sd>, [<Rn>, #<imm>]` | FP Store Register |
| **VLDM** | `VLDM <Rn>{!}, <registers>` | FP Load Multiple |
| **VSTM** | `VSTM <Rn>{!}, <registers>` | FP Store Multiple |
| **VPUSH** | `VPUSH <registers>` | FP Push registers |
| **VPOP** | `VPOP <registers>` | FP Pop registers |

### FP Register Transfer

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| **VMOV** | `VMOV <Rd>, <Sn>` / `VMOV <Sd>, <Rn>` | Move between ARM and FP registers |
| **VMRS** | `VMRS <Rd>, <spec_reg>` | Move from FP system register |
| **VMSR** | `VMSR <spec_reg>, <Rn>` | Move to FP system register |

---

## Standard Assembler Syntax

### Syntax Fields

| Field | Description |
|-------|-------------|
| `<c>` | Condition code (EQ, NE, GT, etc.) - defaults to AL (always) |
| `<q>` | Qualifier: `.N` (narrow/16-bit) or `.W` (wide/32-bit) |
| `<Rd>` | Destination register |
| `<Rn>` | First operand register |
| `<Rm>` | Second operand register |
| `<Ra>` | Accumulator register |
| `<Rt>` | Transfer register (load/store) |
| `<const>` | Immediate constant |

### Shift Options

| Shift | Range | Description |
|-------|-------|-------------|
| `LSL #<n>` | 0-31 | Logical Shift Left |
| `LSR #<n>` | 1-32 | Logical Shift Right |
| `ASR #<n>` | 1-32 | Arithmetic Shift Right |
| `ROR #<n>` | 1-31 | Rotate Right |
| `RRX` | - | Rotate Right with Extend (1 bit) |

### Immediate Constants

- **Modified immediate**: 8-bit value shifted to any position, or repeated byte patterns (Thumb-2 encoding)
- **Plain immediate**: 12-bit value (0-4095) for ADDW/SUBW
- **T1/T2 encodings**: Limited to smaller immediate ranges

---

## Important Notes

### Register Usage
- **R0-R12**: General purpose registers. Safe for arithmetic, logic, and general data storage.
- **R13 (SP)**: Stack Pointer. **CRITICAL: DO NOT overwrite with arbitrary data.** Used by the CPU for stack operations and interrupts; modifying it directly with non-stack data will cause devastating crashes.
- **R14 (LR)**: Link Register. Stores the return address when making subroutine calls (e.g., using `BL`).
- **R15 (PC)**: Program Counter.

### Encoding Notes
- **16-bit instructions**: More compact but limited (only R0-R7 for most operations)
- **32-bit instructions**: Full functionality, use `.W` qualifier to force
- If both encodings available, assembler prefers 16-bit

### Special Cases
- **PC as destination**: Causes branch (interworking for LDR/LDM/POP)
- **SP restrictions**: Some operations limited on SP (shifts only LSL #0-3)
- **Write-back**: Use `!` for pre-indexed write-back

### Exception-Generating Instructions
- **SVC**: Generates SVCall exception (system calls)
- **BKPT**: Generates DebugMonitor exception or halts
- **UDF**: Permanently undefined (generates UsageFault)

### Instruction Nuances & Constraints
This specifies the strict "rules" of the architecture for specific instruction families.
#### 1. Load and Store (Memory ↔ Registers)
- **You CANNOT store an immediate value directly into memory.** `STR` only copies from a *register* to memory.
  - ❌ `STR #0xBAD, [R10]` (Syntax error)
  - ✅ `LDR R8, =0xBAD` then `STR R8, [R10]`
- **Registers must match the transfer size.** Use `STRB`/`LDRB` for 8-bit bytes, `STRH`/`LDRH` for 16-bit halfwords, and `STR`/`LDR` for 32-bit words.
- **Offset vs. Writeback Notation**:
  - `LDR R0, [R1, #4]` -> Loads from `R1+4`. `R1` remains unchanged.
  - `LDR R0, [R1, #4]!` -> Pre-increment: Adds 4 to `R1`, then loads from the new `R1` address.
  - `LDR R0, [R1], #4` -> Post-increment: Loads from `R1`, *then* adds 4 to `R1`. 

#### 2. Data Processing / ALU (Registers ↔ Registers)
- **`MOV` vs `LDR` for Constants**: 
  - `MOV R0, #255` -> Used for small, simple literal constants.
  - `LDR R0, =0xC0DE1` -> A pseudo-instruction used for large 32-bit numbers or memory addresses. The assembler will figure out if it can be a `MOV` or if it must store the constant in a literal pool and PC-relative load it. Always use `LDR =,` for labels/addresses.
- **3-Operand Instructions**: Many ALU instructions take a destination and two sources (`ADD R0, R1, R2` -> `R0 = R1 + R2`). 
- **Multiplication Restrictions**: 
  - In `MUL Rd, Rn, Rm`, `Rd` CANNOT be the same register as `Rm` in older Thumb code, though ARMv7-M is more flexible. Still, keeping them distinct (`MUL R0, R1, R2`) is safest.

#### 3. Comparisons & Conditionals
- **`CMP` does not save a result.** `CMP R0, R1` simply performs `R0 - R1` and updates the CPU's status flags (Zero, Negative, Carry, Overflow). It discards the math result.
- **Chaining Comparisons with `IT` blocks**:
  - If you use an `IT` instruction (e.g., `IT EQ`), the *very next* instruction MUST have the `EQ` suffix (e.g., `ADDEQ` or `STREQ`).
  - Conditionals like `ADDLO` only trigger if the *previous* flag-changing instruction (like `CMP` or `SUBS`) resulted in `LO` (Lower). 

#### 4. Stack Operations
- **`PUSH` and `POP` must be balanced.** Every register pushed inside a subroutine must be popped back off before the subroutine returns, or you will irrevocably corrupt the Stack Pointer (`SP`/`R13`), leading to an instant crash.

### Execution Flow & Syntax Quirks
- **Fall-Through Execution**: The CPU executes instructions continuously from top to bottom. Labels (e.g., `take_mean:`) act purely as "invisible bookmarks" for memory addresses, not boundaries. If you do not explicitly branch over a label or return, the CPU will "fall right through" and execute the code underneath it.
- **Indentation is Ignored**: Unlike Python, assembly completely ignores indentation. Indenting loops/functions is only for programmer readability; the assembler treats the file as a flat sequence of instructions.
- **Loading Variables**: To interact with variables stored in memory, always use a two-step load: first load the *address* (`LDR R1, =VarName`), then load the *value* (`LDR R2, [R1]`). Loading a label directly without `=` can lead to incorrect PC-relative offsets depending on the assembler.
- **Subroutine Returns**: Every subroutine called via `BL` (Branch with Link) must explicitly return using `BX LR` as its final instruction to avoid falling through into undefined memory.
- **Loop Branch Optimization**: When looping with a counter, use flags directly. For example, `SUBS R7, R7, #1` sets the Zero flag when it hits 0. Instead of jumping to the end when it equals 0, just use `BNE top_of_loop` to branch back if not zero, allowing the loop to naturally fall-through when it finishes.

---

## Quick Reference: Common Patterns

### Function Call
```asm
BL  function      ; Call subroutine
...               ; Return here
function:
    ...           ; Function code
    BX  LR        ; Return
```

### Conditional Execution
```asm
CMP  R0, R1       ; Compare R0 with R1
ITTE GT           ; If-Then-Then-Else Greater Than
MOVGT R2, #1      ; R2 = 1 if R0 > R1
MOVGT R3, #2      ; R3 = 2 if R0 > R1
MOVLE R2, #0      ; R2 = 0 if R0 <= R1
```

### Load/Store with Write-back
```asm
LDR R0, [R1], #4  ; Post-increment: load from [R1], then R1 += 4
LDR R0, [R1, #4]! ; Pre-increment: R1 += 4, then load from [R1]
```

### Stack Operations
```asm
PUSH {R0-R3, LR}  ; Save registers and return address
...               ; Function body
POP  {R0-R3, PC}  ; Restore registers and return
```

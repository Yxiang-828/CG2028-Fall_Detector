# CG2028 FALL DETECTION SYSTEM: PROJECT GUIDE & DOCUMENTATION

This document serves as your comprehensive guide to the **CG2028 Assignment**. It includes project specifications, step-by-step implementation details, verification methods, and hardware documentation.

---

## 1. PROJECT OVERVIEW

**Goal:** Design a "smart" fall detection system using the **STM32L4S5 Discovery Board**.

**Problem:** The **"Long Lie"** — elderly people falling and remaining undetected for hours. This delay leads to a 50% mortality rate within six months.
**Solution:** A wearable device that detects falls using accelerometer and gyroscope data, and signals for help via LED, Buzzer, and OLED display. It must be smarter than gravity to distinguish falls from normal movements.

### **System Architecture**

1.  **Input:**
    *   **Accelerometer:** Measures linear acceleration ($m/s^2$ or $g$). Detects impact spikes.
    *   **Gyroscope:** Measures angular velocity ($dps$). Detects rapid rotation (falling over).
    *   **User Input:** 5-Way Joystick for system control (Arm/Disarm).

2.  **Processing:**
    *   **Assembly (`mov_avg.s`):** A specialized Moving Average Filter for noise reduction.
    *   **C Host (`main.c`):** Sensor management, Fall Detection Logic, UART reporting.

3.  **Output:**
    *   **Visual:** LED (Blink codes), OLED Display (Status faces).
    *   **Audio:** Active Buzzer (Alarm).
    *   **Data:** UART Transmission to PC (Data logging).

---

## 2. HOW TO RUN & VERIFY (CRITICAL SETUP)

This workspace contains two distinct projects. You must know which one to run.

### **A. `CG2028_Assignment_Test` (The Simulation)**
*   **Purpose:** Verifies your assembly logic (`mov_avg.s`) using fake, hardcoded numbers.
*   **Where to see output:** The **IDE Console** window (via Semihosting).
*   **How to Run:**
    1.  Right-click `CG2028_Assignment_Test` -> **Debug As** -> **STM32 Cortex-M C/C++ Application**.
    2.  Click **Resume (F8)**.
    3.  Check the "Console" tab at the bottom.
    4.  **Success:** It prints `Test passed`.

### **B. `CG2028_Assignment` (The Real Application)**
*   **Purpose:** Runs on the physical board with real sensors.
*   **Where to see output:** **Tera Term** (Serial Monitor). You will **NOT** see output in the IDE Console.
*   **How to Setup Tera Term:**
    1.  Plug in your STM32 Board via USB.
    2.  Open **Tera Term**.
    3.  Select **Serial** (Radio button).
    4.  Choose the Port labeled **"STMicroelectronics STLink Virtual COM Port"**. Click OK.
    5.  Go to **Setup** -> **Serial Port...**
    6.  Set **Speed (Baud rate)** to **115200**.
    7.  Click **New Setting**.
*   **How to Run:**
    1.  Right-click `CG2028_Assignment` -> **Debug As** -> **STM32 Cortex-M C/C++ Application**.
    2.  Click **Resume (F8)**.
    3.  Look at the Tera Term window.
    4.  **Success:** You see scrolling data: `Acc: 0.12, Gyro: 0.05` and `FALL DETECTED` messages when shaken.

---

## 3. VERIFICATION: THE TEST BENCH (`CG2028_Assignment_Test`)

Before integrating with real sensors, you must verify your assembly filter works correctly using the provided test bench. Real sensors are noisy; if your filter is broken (e.g., misaligned stack, wrong register usage), the whole system fails unpredictably. This test bench isolates the logic.

### **How the Test Works**
The test project simulates sensor data coming in one by one. It feeds a known array of numbers into your filter and checks if the output matches the reference C implementation.

**Circular Buffer Logic:** The test fills a buffer `{0, 0, 0, 0}` one slot at a time.
*   *Iteration 1 (Input 1000):* Buffer `{1000, 0, 0, 0}` $\rightarrow$ Sum 1000 $\rightarrow$ Avg **250**.
*   *Iteration 2 (Input 1001):* Buffer `{1000, 1001, 0, 0}` $\rightarrow$ Sum 2001 $\rightarrow$ Avg **500**.
*   *Iteration 3 (Input 1002):* Buffer `{1000, 1001, 1002, 0}` $\rightarrow$ Sum 3003 $\rightarrow$ Avg **750**.
*   *Iteration 4 (Input 1003):* Buffer `{1000, 1001, 1002, 1003}` $\rightarrow$ Sum 4006 $\rightarrow$ Avg **1001**.

### **The Test Cases (Provided in `main.c`)**

#### **Test Case 1: Ramp Up**
*   **Input X:** `{1000, 1001, 1002, ...}`
*   **Buffer Behavior:** Slowly fills with increasing numbers.
*   **Expected Output:** `{250, 254, 258}`, `{500, 508, 516}`, ...
*   **Why Needed:** verify the filter handles "warm-up" correctly when the buffer is partially empty (still contains initial 0s).

#### **Test Case 2: Zero Check**
*   **Input:** All `{0, 0, 0...}`
*   **Expected Output:** All `{0}`.
*   **Why Needed:** Ensures the filter doesn't introduce bias or offset errors.

#### **Test Case 3: Random Real Data**
*   **Input:** Real accelerometer data dump (noisy numbers like `{6030, 6000, 4389...}`).
*   **Expected Output:** A smoothed version of the input.
*   **Why Needed:** Verifies the filter works with actual large numbers and doesn't overflow intermediate calculations.

---

## 3. PART 1: THE ASSEMBLY FILTER (`mov_avg.s`)

This is the core signal processing unit. It must be efficient and follow the **ARM Procedure Call Standard (AAPCS)**.

### **Specification**
*   **Prototype:** `extern int mov_avg(int N, int* accel_buff);`
*   **Registers:**
    *   `R0`: Window Size **N** (4).
    *   `R1`: Pointer to the **buffer** (int array).
    *   `R0` (Return): The calculated average.

### **Correct Implementation Code**
*Copy this into `Core/Src/mov_avg.s` in both Test and Main projects.*

```assembly
/*
 * mov_avg.s
 * Moving Average Filter in ARM Assembly
 */
.syntax unified
.cpu cortex-m4
.thumb
.global mov_avg

@ -------------------------------------------------------------------
@ Function: mov_avg
@ R0: N (Window Size, e.g., 4)
@ R1: int* accel_buff (Pointer to buffer)
@ Return: R0 (Average)
@ -------------------------------------------------------------------

mov_avg:
    @ 1. Save Context
    @ Pushing R2-R11, LR ensures correct register preservation
    PUSH {R2-R11, LR}

    MOV R2, #0          @ R2 = loop counter (i = 0)
    MOV R3, #0          @ R3 = accumulator (sum = 0)

loop:
    CMP R2, R0          @ Compare i with N (R0)
    BGE end_loop        @ If i >= N, exit loop

    LDR R4, [R1, R2, LSL #2]  @ Load accel_buff[i] into R4. (Offset = i * 4 bytes)
    ADD R3, R3, R4      @ sum += accel_buff[i]

    ADD R2, R2, #1      @ i++
    B loop              @ Repeat loop

end_loop:
    ASR R0, R3, #2      @ R0 = sum / 4 (Arithmetic Shift Right by 2 = divide by 4)
    
    POP {R2-R11, PC}    @ Restore registers and return

    MOV R4, R0          @ R4 = N (Copy count for division later)
    MOV R5, R0          @ R5 = Loop Counter (starts at N)
    MOV R6, #0          @ R6 = Sum Accumulator

loop:
    CMP R5, #0          @ Check if loop counter is 0
    BEQ done            @ If 0, finished summing

    LDR R7, [R1], #4    @ Load value from [R1] into R7, then R1 = R1 + 4
    ADD R6, R6, R7      @ Sum = Sum + Loaded Value

    SUB R5, R5, #1      @ Decrement counter
    B loop              @ Repeat

done:
    @ 2. Calculate Average
    SDIV R0, R6, R4     @ R0 = R6 / R4 (Sum / N)

    @ 3. Restore Context
    @ Must match the PUSH list exactly
    POP {R3-R11, PC}
```

---

## 4. PART 2: THE MAIN APPLICATION (`main.c`)

The C code manages the sensors, buffers, and fallback detection decisions. Moving away from naive threshold checking, the system features a **4-Stage Biomechanical Physics Engine** designed to rigorously distinguish genuine human falls from false positive anomalies (like bumping the desk or dropping the device on a bed).

### **Core Data Management**
The system maintains a non-blocking **50Hz hardware polling loop** using `HAL_GetTick()`. Circular buffers hold the 4 most recent `$x, y, z$` acceleration readings:
```c
int accel_buff_x[4] = {0};
// Index increments continuously: x_buff[i % 4] = new_reading;
```
This guarantees an ultra-low latency response while still smoothing data through the assembly Moving Average Filter.

### **The 4-Stage Finite State Machine (FallState)**
A real human fall is a chronological progression of physical events. The `current_fall_state` tracks this exact progression:

#### **Stage 1: FREEFALL (The Drop)**
*   **Physics:** When a person faints or their legs buckle, they accelerate downwards, neutralizing gravity.
*   **Trigger:** If `total_accel < 7.0 m/s^2` (< 0.7G).
*   **Action:** Starts a `freefall_start_time` crash timer and waits.

#### **Stage 2: TUMBLE (The Uncontrolled Descent)**
*   **Physics:** As a human falls over 0.5 to 1.5 seconds, their body *unavoidably* tips, flails, or rotates. We use this to filter out jumping straight down.
*   **Trigger:** If `total_gyro > 50.0 dps` occurs *during* the freefall timer.
*   **Action:** Confirms human descent. (If 1.5s passes without rotation, it's a False Alarm).

#### **Stage 3: IMPACT (The Deceleration Spike)**
*   **Physics:** The body hits the ground, causing a massive deceleration shockwave. 
*   **Trigger:** If `total_accel > 15.0 m/s^2` (> 1.5G) occurs while tumbling.
*   **Action:** Triggers the ALARM. (If 2.0s passes after the drop without an impact, the user recovered their balance).

#### **Stage 4: ALARM (Emergency Protocol)**
*   **Action:** The system locks into a high-visibility, 10Hz fast-strobe LED state `led_blink_interval = 100` and floods the UART terminal with critical warning messages. It holds this state for 5 seconds to simulate an emergency beacon.

---

## 5. HARDWARE ENHANCEMENTS AND PERIPHERALS

This section details the extra hardware used, how it works, and why it is essential for a complete safety system.

### **A. 5-Way Joystick Center Button (System Arm/Disarm)**
*   **Component:** On-board Joystick (Center Pin connected to `PA1`).
*   **Function:** Functions as the master ON/OFF switch for the detection logic.
*   **How it Works:** The joystick is a digital input switch. When pressed, it connects the circuit, pulling the GPIO Pin `PA1` HIGH (`1`). The software detects this "Rising Edge".
*   **Why we want it:** Usability. Real users need to take the device off (e.g., to shower or sleep). Without a way to "Disarm" the system, simply placing the device on a table or dropping it onto a bed would trigger false alarms. This prevents the "Boy Who Cried Wolf" scenario.

### **B. Active Buzzer (Audible Alarm)**
*   **Component:** Piezo-electric buzzer connected to `PA0`.
*   **Function:** Emits a loud beep when a fall is confirmed.
*   **How it Works:** An *Active* buzzer has an internal oscillator. It generates sound as long as it receives a DC voltage (High Signal).
    *   **Logic High (3.3V):** `HAL_GPIO_WritePin(...SET)` $\rightarrow$ **BEEP**.
    *   **Logic Low (0V):** `HAL_GPIO_WritePin(...RESET)` $\rightarrow$ **SILENT**.
*   **Why we want it:** The **"Long Lie" Solution**. If a senior falls and loses consciousness (or breaks their glasses), they cannot see an LED blinking or read an OLED screen. Sound is the only way to alert caregivers in another room.

### **C. OLED Display (SSD1306 - I2C)**
*   **Component:** 0.96" OLED Screen (128x64 pixels).
*   **Protocol:** **I2C** (Inter-Integrated Circuit). Uses 2 wires: `SDA` (Data) and `SCL` (Clock).
*   **How it Works:** The STM32 sends pixel data packets to address `0x78`. The display controller turns individual organic LEDs on/off to form images.
*   **Software Logic:**
    *   `Show_Happy_Face()`: Draws a curve and eyes. Indicates **"Monitoring - Safe"**.
    *   **Fall:** Switch to `Show_Angry_Face()` (X eyes). Indicates **"DANGER"**.
*   **Why we want it:** Psychological Reassurance. A blinking LED is cryptic. A "Happy Face" provides immediate, human-readable confirmation that the system is watching over them.

### **D. User LED Indicator (LED2)**
*   **Component:** On-board Green LED (`PB14`).
*   **Function:** Provides low-power, instant status.
*   **Logic:**
    *   **Fall:** `BSP_LED_Toggle` with `HAL_Delay(50)` $\rightarrow$ Fast Strobe (10Hz).
    *   **Safe:** Off or Slow Pulse.
*   **Why we want it:** Reliability. If the OLED cracks during the fall or the buzzer wire disconnects, the on-board LED is the fallback diagnostic tool for the user or technician.

---

## 6. WIRING GUIDE (Arduino Header)

| Component | Pin Function | STM32 Pin | Arduino Header Label |
| :--- | :--- | :--- | :--- |
| **Buzzer (+)** | GPIO Output | **PA0** | **A0 or D0** (Check Setup) |
| **Buzzer (-)** | Ground | **GND** | **GND** |
| **Switch (Center)**| GPIO Input | **PA1** | **A1 or D1** |
| **OLED (SCL)** | I2C1_SCL | **PB8** | **D15 (SCL)** |
| **OLED (SDA)** | I2C1_SDA | **PB9** | **D14 (SDA)** |
| **OLED (VCC)** | Power | **3.3V** | **3.3V** |
| **OLED (GND)** | Ground | **GND** | **GND** |

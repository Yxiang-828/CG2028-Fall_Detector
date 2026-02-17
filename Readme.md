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

## 2. VERIFICATION: THE TEST BENCH (`CG2028_Assignment_Test`)

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
    @ Pushing R3-R11, LR ensures 8-byte stack alignment (10 registers = 40 bytes)
    PUSH {R3-R11, LR}

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

The C code manages the sensors, buffers, and fall detection decisions.

### **Global Variables (Top of File)**

```c
/* USER CODE BEGIN PV */
// Sensor Buffers (Circular)
int x_buff[4] = {0}, y_buff[4] = {0}, z_buff[4] = {0};
int buf_index = 0;

// System Flags
int system_armed = 1;     // 1=ON (Monitoring), 0=OFF (Standby)
int fall_confirmed = 0;   // 1=FALL DETECTED
int possible_fall_wait = 0; // State machine for fall sequence

// Debouncing
uint32_t last_btn_time = 0;
/* USER CODE END PV */
```

### **Main Loop Logic**

```c
/* USER CODE BEGIN 3 */
    // === 1. SYSTEM CONTROL (5-Way Switch) ===
    // PA1 is the Center Button of the joystick. 
    // Pressing it toggles the system ON or OFF.
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) {
        if (HAL_GetTick() - last_btn_time > 200) { // 200ms Debounce
            system_armed = !system_armed; // Toggle State
            fall_confirmed = 0;           // Reset Alarms
            last_btn_time = HAL_GetTick();
        }
    }

    if (system_armed) {
        // === 2. SENSOR ACQUISITION ===
        int16_t pData[3] = {0};
        BSP_ACCELERO_AccGetXYZ(pData);

        // Update Circular Buffers
        x_buff[buf_index % 4] = pData[0];
        y_buff[buf_index % 4] = pData[1];
        z_buff[buf_index % 4] = pData[2];
        buf_index++;

        // === 3. FILTERING (Assembly Call) ===
        // Get average from assembly, convert to float (m/s^2)
        float ax = (float)mov_avg(4, x_buff) * (9.8/1000.0f);
        float ay = (float)mov_avg(4, y_buff) * (9.8/1000.0f);
        float az = (float)mov_avg(4, z_buff) * (9.8/1000.0f);
        
        // === 4. FALL DETECTION LOGIC ===
        // Calculate Total Magnitude Vector: sqrt(x^2 + y^2 + z^2)
        float magnitude = sqrt(ax*ax + ay*ay + az*az);

        // Threshold 1: Free Fall (Magnitude drops < 3.0 m/s^2)
        if (magnitude < 3.0) {
             possible_fall_wait = 1;
        }

        // Threshold 2: Impact (Magnitude spikes > 20.0 m/s^2)
        // A fall is usually Free Fall -> Impact
        if (possible_fall_wait && magnitude > 20.0) {
            fall_confirmed = 1;
            possible_fall_wait = 0;
        }

        // === 5. OUTPUTS (Visual/Audio) ===
        if (fall_confirmed) {
            BSP_LED_Toggle(LED2);               // FAST Blink (Panic Mode)
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); // Buzzer ON
            Show_Angry_Face();                  // OLED: "FALL DETECTED!"
            HAL_Delay(50);                      // Fast loop speed
        } else {
            BSP_LED_Off(LED2);                  // LED OFF (Normal)
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET); // Buzzer OFF
            Show_Happy_Face();                  // OLED: "SYSTEM OK"
            HAL_Delay(50);                      // Normal loop speed
        }

        // === 6. DATA LOGGING (UART) ===
        // Print immediately (index >= 0) to debug
        if (buf_index >= 0) { 
             char buffer[100];
             // Requires -u _printf_float in Linker Settings
             sprintf(buffer, "Acc: %.2f, %.2f, %.2f | Mag: %.2f\r\n", ax, ay, az, magnitude);
             HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
        }
        
    } else {
        // === STANDBY MODE ===
        BSP_LED_Off(LED2);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        // Show_Standby_Face();
        HAL_Delay(100);
    }
/* USER CODE END 3 */
```

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

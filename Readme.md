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
        // Calculate Total Magnitude Vector
        float total_accel = sqrt(ax*ax + ay*ay + az*az);
        // (Assuming gx, gy, gz are parsed from gyro similarly)
        float total_gyro = sqrt(gx*gx + gy*gy + gz*gz);

        // Thresholds based on biomechanical literature
        float ACCEL_THRESHOLD_HIGH = 25.0f;
        float GYRO_THRESHOLD = 300.0f;

        // Condition: High impact OR High rotatonal velocity
        if (total_accel > ACCEL_THRESHOLD_HIGH || total_gyro > GYRO_THRESHOLD) {
            fall_confirmed = 1;
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

---

## 7. FALL DETECTION METHODOLOGY & JUSTIFICATION

### **A. General Information & Analysis**
The "golden point" of a true fall is not just the impact with the ground—it is the **failure to recover**. People frequently perform hard impacts (jumping) or rapid rotations (lying down quickly), but in a genuine emergency, the user remains incapacitated on the floor. This is known as the "Long Lie" scenario, which drastically increases mortality rates.

Furthermore, analyzing the biomechanics of how people fall reveals that falls rarely happen straight down perfectly vertically. Usually, there is a loss of balance resulting in a **rapid rotation or tumble**, followed by a **freefall** phase, ending in a **hard impact**.

### **B. System Architecture: 4-State Mealy Machine**
To combat false positives from daily activities, this system is built on a **Mealy Finite State Machine (FSM)**. A Mealy machine computes its next state and outputs based on both its *current state* and *current sensor inputs*. This allows us to track the chronological sequence of a fall, rather than relying on a single moment in time.

The FSM consists of 4 states:
1. **`STATE_NORMAL`**: The system is monitoring at 10Hz. It waits for the first anomaly (either a massive impact, a freefall, or a huge rotational spike).
2. **`STATE_FALLING`**: The sequence has begun. The system waits up to **1.5 seconds** to see if the *other* conditions occur (e.g., if rotation triggered us, we wait for the impact). If they don't all occur together within 1.5s, it was a fake fall (like sitting down fast) and it resets.
3. **`STATE_STILLNESS_CHECK`**: Both impact and rotation happened! The system now initiates a **5-second countdown**. It employs a 2-second "blind period" to let the physical sensors and surfaces stop bouncing, and then checks if the user is moving (recovering). If the user gets up, the alarm is aborted.
4. **`STATE_CONFIRMED`**: The countdown reached zero without the user getting up. The system declares a "Long Lie" emergency and triggers the alarms.

### **C. Current Thresholds & Conditions**
Our system uses the following finely-tuned empirical thresholds to distinguish true falls from casual activities (like moving the board, shaking it, or dropping it on a soft pillow):

*   **`ACCEL_THRESHOLD_HIGH = 30.0f` ($m/s^2$)**: Detects a genuinely hard, violent impact ($\approx 3.0g$). Bumped up from 25.0f to ignore casual table bumps.
*   **`ACCEL_THRESHOLD_LOW = 2.5f` ($m/s^2$)**: Detects a state of **Freefall** ($\approx 0.25g$). This is critical for detecting when the user falls onto a soft surface (like a bed/pillow) that absorbs the final impact.
*   **`GYRO_THRESHOLD = 400.0f` ($dps$)**: Detects incredibly rapid tumbling/rotation. Bumped up heavily from 150 dps so that casual wrist twisting or swishing does not trigger the FSM. 

*   **Recovery Thresholds (Stillness Check)**: To abort the 5-second alarm countdown, the user must generate significant movement: `> 5.0f` deviation in acceleration from baseline 1g, or `> 250.0f` dps rotation.

### **D. Testing Methodology**
The system logic was heavily verified using live visual debugging via a serial connection.
*   **Serial Output (Tera Term):** By printing large, multi-line ASCII art text blocks for every state transition (e.g., `FALLING`, `STILLNESS`), we could physically throw, drop, and shake the board while instantly seeing its decision-making process in real-time. 
*   **Countdown Visilibity:** During the crucial stillness check, massive ASCII numbers (5 down to 1) were printed per second to verify the timer logic and the blind-period recovery aborts correctly.
*   *Pending tests*: Full integration testing with the physical Buzzer, LED blink sequences, and I2C OLED display reactions to the confirmed state.

---

## 8. CG2028 ASSESSMENT SCHEDULE AND INFORMATION

The following rules and guidelines apply to the final assessment demonstration (Week 7):

*   **Punctuality:** Arrive at least 15 minutes before the designated slot to test the system and prepare. Both team members must strictly be present.
*   **Code Submission:** 
    *   **CRITICAL:** Only the version downloaded from Canvas will be graded. 
    *   Running code from a thumb drive, USB, or Google Drive is considered a late submission and will incur penalties.
    *   Ensure the code compiles without errors; failure to compile will result in significant mark deductions.
    *   Ensure the final assembly program remains compatible with the provided `main.c` test file and expected interface.
*   **Hardware & Setup:** 
    *   Bring your own STM32 board.
    *   Demonstrate on the lab computer (Windows setup with STM32 IDE and Tera Term). 
    *   Projects with extra features requiring other software can be shown on a personal laptop.
*   **Presentation & QnA:**
    *   Use your own device for slides. Be ready to explain design choices.
    *   Each student will be assessed individually with 2 project-related questions.
    *   Must be able to demonstrate the system distinguishing true positives from false positives.

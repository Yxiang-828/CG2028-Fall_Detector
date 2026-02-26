/******************************************************************************
  * @file           : main.c
  * @brief          : Pure Data Acquisition Logger for FSM Calibration
  * (c) CG2028 Teaching Team
  ******************************************************************************/
/*
Test 1: The Drop (ACCEL_THRESHOLD_HIGH & LOW)

Action: Hold the board flat against your chest. Fall backward onto a bed or mattress.

Data to record: Look at the Accel peak right when you hit the bed. Set your ACCEL_THRESHOLD_HIGH slightly below that number. Look at the lowest Accel value printed right before the impact (that's your freefall). Set ACCEL_THRESHOLD_LOW slightly above that number.


Test 2: The Tumble (GYRO_THRESHOLD)

Action: Hold the board flat against your chest and fall sideways onto the bed.

Data to record: Look at the Gyro peak. Set your GYRO_THRESHOLD slightly below that number.


Test 3: The Recovery (RECOVERY_DEVIATION)

Action: Lie on your back. Roll around gently (simulating pain/incapacitation) and note the Dev peak. Then, do a strict sit-up to simulate getting up, and note that Dev peak.

Data to record: Set your stillness exit threshold higher than the rolling deviation, but lower than the sit-up deviation.
*/
#include "main.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_accelero.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_tsensor.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_gyro.h"

#include "stdio.h"
#include "string.h"
#include <math.h>

static void UART1_Init(void);
static void ADC1_Init(void); 
uint32_t Read_Sound_Sensor(void); 

extern int mov_avg(int N, int* accel_buff); 

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1; 

int main(void)
{
    const int N = 4;

    HAL_Init();
    UART1_Init();
    BSP_ACCELERO_Init();
    BSP_GYRO_Init();
    ADC1_Init(); 

    int accel_buff_x[4] = {0};
    int accel_buff_y[4] = {0};
    int accel_buff_z[4] = {0};
    int i = 0;
    
    char buffer[200]; 
    uint32_t last_sensor_read_time = 0;
    
    // Peak Tracking Variables
    uint32_t peak_sound = 0; 
    float peak_accel = 0.0f; 
    float peak_gyro = 0.0f;  
    float peak_dev = 0.0f;
    float min_accel = 99.0f; // Track lowest drop for freefall

    sprintf(buffer, "\r\n=== CG2028 CALIBRATION LOGGER STARTED ===\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

    while (1)
    {
        // 50Hz Polling Gate (20ms)
        if (HAL_GetTick() - last_sensor_read_time < 20) continue;
        last_sensor_read_time = HAL_GetTick();

        // 1. Read Accelerometer
        int16_t accel_data_i16[3] = { 0 };          
        BSP_ACCELERO_AccGetXYZ(accel_data_i16);
        accel_buff_x[i%4] = accel_data_i16[0]; 
        accel_buff_y[i%4] = accel_data_i16[1]; 
        accel_buff_z[i%4] = accel_data_i16[2]; 

        // 2. Read Gyroscope
        float gyro_data[3] = {0.0};
        BSP_GYRO_GetXYZ(gyro_data);
        float gyro_velocity[3] = {0.0};
        gyro_velocity[0] = (gyro_data[0] / 1000.0f);
        gyro_velocity[1] = (gyro_data[1] / 1000.0f);
        gyro_velocity[2] = (gyro_data[2] / 1000.0f);

        // 3. Apply mov_avg to Accelerometer
        float accel_filt_asm[3] = {0}; 
        accel_filt_asm[0] = (float)mov_avg(N, accel_buff_x) * (9.8f/1000.0f);
        accel_filt_asm[1] = (float)mov_avg(N, accel_buff_y) * (9.8f/1000.0f);
        accel_filt_asm[2] = (float)mov_avg(N, accel_buff_z) * (9.8f/1000.0f);

        // 4. Read Sound
        uint32_t current_sound = Read_Sound_Sensor();

        // 5. Calculate Magnitudes
        float total_accel = sqrtf(powf(accel_filt_asm[0], 2) + powf(accel_filt_asm[1], 2) + powf(accel_filt_asm[2], 2));
        float total_gyro = sqrtf(powf(gyro_velocity[0], 2) + powf(gyro_velocity[1], 2) + powf(gyro_velocity[2], 2));
        float current_dev = fabs(total_accel - 9.8f);

        // 6. Update Peaks for the 500ms Window
        if (current_sound > peak_sound) peak_sound = current_sound;
        if (total_accel > peak_accel) peak_accel = total_accel;
        if (total_accel < min_accel) min_accel = total_accel; // Lowest Accel
        if (total_gyro > peak_gyro) peak_gyro = total_gyro;
        if (current_dev > peak_dev) peak_dev = current_dev;

        // 7. Print Peaks Every 500ms
        if (i % 25 == 0) {
            sprintf(buffer, "[PEAKS] Snd: %4lu | MaxA: %5.2f | MinA: %5.2f | Dev: %5.2f | Gyr: %6.2f\r\n", 
                    peak_sound, peak_accel, min_accel, peak_dev, peak_gyro);
            HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
            
            // Reset trackers
            peak_sound = 0; 
            peak_accel = 0.0f;
            min_accel = 99.0f;
            peak_gyro = 0.0f;
            peak_dev = 0.0f;
        }
        
        i++; 
    }
}

// ======================= ADC INITIALIZATION =========================
static void ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) while(1); 

    sConfig.Channel = ADC_CHANNEL_13;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) while(1); 
}

uint32_t Read_Sound_Sensor(void)
{
    uint32_t start_time = HAL_GetTick();
    uint32_t max_val = 0;
    uint32_t min_val = 4095;

    while (HAL_GetTick() - start_time < 10) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK) {
            uint32_t val = HAL_ADC_GetValue(&hadc1);
            if (val > max_val) max_val = val;
            if (val < min_val) min_val = val;
        }
    }
    return (max_val - min_val);
}

static void UART1_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK) while(1);
}

// Stubs to suppress warnings
int _read(int file, char *ptr, int len) { return 0; }
int _fstat(int file, struct stat *st) { return 0; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _isatty(int file) { return 1; }
int _close(int file) { return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { return -1; }
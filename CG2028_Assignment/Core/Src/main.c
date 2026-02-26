/******************************************************************************
  * @file           : main.c
  * @brief          : Fall Detection FSM (Silent Alarm Loop & Sentinel)
  * (c) CG2028 Teaching Team / Assistant Aiko 💕
  ******************************************************************************/

#include "main.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_accelero.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_tsensor.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_gyro.h"

#include "stdio.h"
#include "string.h"
#include <sys/stat.h>
#include <math.h>

static void UART1_Init(void);
static void Buzzer_GPIO_Init(void);
static void Button_GPIO_Init(void);
static void ADC1_Init(void); 
uint32_t Read_Sound_Sensor(void); 

extern void initialise_monitor_handles(void);   

extern int mov_avg(int N, int* accel_buff); 
int mov_avg_C(int N, int* accel_buff); 

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1; 

typedef enum {
    STATE_NORMAL = 0,
    STATE_FALLING,
    STATE_STILLNESS_CHECK,
    STATE_CONFIRMED
} FallState_t;

FallState_t current_state = STATE_NORMAL;
uint32_t state_timer = 0;

int seen_impact = 0;
int seen_rotation = 0;
int seen_freefall = 0; 
int seen_loud_noise = 0; 

int system_armed = 1; 

int main(void)
{
    const int N=4;

    HAL_Init();
    UART1_Init();
    BSP_LED_Init(LED2);
    BSP_ACCELERO_Init();
    BSP_GYRO_Init();

    Buzzer_GPIO_Init();
    Button_GPIO_Init();
    ADC1_Init(); 

    BSP_LED_Off(LED2);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 

    int accel_buff_x[4]={0};
    int accel_buff_y[4]={0};
    int accel_buff_z[4]={0};
    int i=0;
    int delay_ms=20; 
    char buffer[600]; 

    int btn_press_count = 0;
    uint32_t btn_first_press_time = 0;
    uint32_t btn_last_debounce_time = 0;
    int btn_last_state = 1; 
    int btn_waiting_for_decision = 0;

    uint32_t last_sensor_read_time = 0;
    
    uint32_t peak_sound_window = 0; 
    float peak_accel_window = 0.0f; 
    float peak_gyro_window = 0.0f;  
    
    uint32_t sound_history[3] = {0, 0, 0}; 
    uint32_t bg_sound_max = 0;

    while (1)
    {
        // ========== MULTI-PRESS BUTTON HANDLER ==========
        int btn_current = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

        if (btn_current == GPIO_PIN_RESET && btn_last_state == 1 && (HAL_GetTick() - btn_last_debounce_time > 50)) {
            btn_press_count++;
            btn_last_debounce_time = HAL_GetTick();
            if (btn_press_count == 1) btn_first_press_time = HAL_GetTick(); 
            btn_waiting_for_decision = 1;
        }
        btn_last_state = (btn_current == GPIO_PIN_SET) ? 1 : 0;

        if (btn_waiting_for_decision && (HAL_GetTick() - btn_first_press_time > 500)) {
            btn_waiting_for_decision = 0;

            if (btn_press_count == 1) {
                if (current_state == STATE_CONFIRMED) {
                    current_state = STATE_NORMAL;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 
                    BSP_LED_Off(LED2);
                    sprintf(buffer, "\r\n--- ALARM RESET (1 press) ---\r\n");
                    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                }
            }
            else if (btn_press_count == 2) {
                system_armed = !system_armed;
                current_state = STATE_NORMAL; 
                BSP_LED_Off(LED2);
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 

                int beeps = system_armed ? 1 : 2;
                for (int b = 0; b < beeps; b++) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
                    HAL_Delay(80);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
                    if (b < beeps - 1) HAL_Delay(80); 
                }

                if (system_armed) sprintf(buffer, "\r\n--- SYSTEM ARMED (2 presses) ---\r\n");
                else sprintf(buffer, "\r\n--- SYSTEM DISARMED (2 presses) ---\r\n");
                
                HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
            }
            else if (btn_press_count >= 3) {
                system_armed = 1; 
                current_state = STATE_CONFIRMED;
                sprintf(buffer, 
                    "\r\n___SEND_TELEGRAM_ALERT___\r\n"
                    "!!! MANUAL ALARM TRIGGERED (3 presses) !!!\r\n");
                HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
            }
            btn_press_count = 0; 
        }

        // ========== NON-BLOCKING SENSOR GATE ==========
        if (HAL_GetTick() - last_sensor_read_time < (uint32_t)delay_ms) continue;
        last_sensor_read_time = HAL_GetTick();

        if (!system_armed) {
            delay_ms = 500; 
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 
            BSP_LED_Off(LED2);
            continue; 
        }

        int16_t accel_data_i16[3] = { 0 };          
        BSP_ACCELERO_AccGetXYZ(accel_data_i16);

        accel_buff_x[i%4]=accel_data_i16[0]; 
        accel_buff_y[i%4]=accel_data_i16[1]; 
        accel_buff_z[i%4]=accel_data_i16[2]; 

        float gyro_data[3]={0.0};
        BSP_GYRO_GetXYZ(gyro_data);

        float gyro_velocity[3]={0.0};
        gyro_velocity[0] = (gyro_data[0] / 1000.0f);
        gyro_velocity[1] = (gyro_data[1] / 1000.0f);
        gyro_velocity[2] = (gyro_data[2] / 1000.0f);

        float accel_filt_asm[3]={0}; 
        accel_filt_asm[0]= (float)mov_avg(N,accel_buff_x) * (9.8f/1000.0f);
        accel_filt_asm[1]= (float)mov_avg(N,accel_buff_y) * (9.8f/1000.0f);
        accel_filt_asm[2]= (float)mov_avg(N,accel_buff_z) * (9.8f/1000.0f);

        uint32_t current_sound = Read_Sound_Sensor();

        float total_accel = sqrtf(powf(accel_filt_asm[0], 2) + powf(accel_filt_asm[1], 2) + powf(accel_filt_asm[2], 2));
        float total_gyro = sqrtf(powf(gyro_velocity[0], 2) + powf(gyro_velocity[1], 2) + powf(gyro_velocity[2], 2));

        if (current_sound > peak_sound_window) peak_sound_window = current_sound;
        if (total_accel > peak_accel_window) peak_accel_window = total_accel;
        if (total_gyro > peak_gyro_window) peak_gyro_window = total_gyro;

        float ACCEL_THRESHOLD_HIGH = 20.0f; 
        float ACCEL_THRESHOLD_LOW = 5.0f;   
        float GYRO_THRESHOLD = 400.0f;      

        static int last_printed_second = -1;

        // 500ms Print and Baseline Update (Silenced during ALARM)
        if (i % 25 == 0) {
            if (current_state != STATE_CONFIRMED) {
                sprintf(buffer, "Sound:%lu | Accel:%.2f | Gyro:%.2f\r\n", peak_sound_window, peak_accel_window, peak_gyro_window);
                HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
            }
            
            sound_history[2] = sound_history[1];
            sound_history[1] = sound_history[0];
            sound_history[0] = peak_sound_window;

            bg_sound_max = sound_history[0];
            if (sound_history[1] > bg_sound_max) bg_sound_max = sound_history[1];
            if (sound_history[2] > bg_sound_max) bg_sound_max = sound_history[2];

            peak_sound_window = 0; 
            peak_accel_window = 0.0f;
            peak_gyro_window = 0.0f;
        }
        
        i++; 

        // ********* Fall Detection FSM *********/
        switch (current_state) {
            case STATE_NORMAL:
                seen_impact = 0;
                seen_rotation = 0;
                seen_freefall = 0;
                seen_loud_noise = 0;
                delay_ms = 20; 

                if (total_accel > ACCEL_THRESHOLD_HIGH) seen_impact = 1;
                if (total_accel < ACCEL_THRESHOLD_LOW)  seen_freefall = 1;
                if (total_gyro  > GYRO_THRESHOLD)       seen_rotation = 1;

                if (seen_impact || seen_freefall || seen_rotation) {
                    current_state = STATE_FALLING;
                    state_timer = HAL_GetTick();

                    sprintf(buffer,
                        "\r\n=================================\r\n"
                        " EVENT DETECTED - INVESTIGATING...\r\n"
                        " Trigger: %s%s%s\r\n"
                        "=================================\r\n",
                        seen_impact ? "IMPACT " : "",
                        seen_freefall ? "FREEFALL " : "",
                        seen_rotation ? "ROTATION" : "");
                    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                }
                break;

            case STATE_FALLING:
                delay_ms = 20; 

                if (total_accel > ACCEL_THRESHOLD_HIGH) seen_impact = 1; 
                if (total_accel < ACCEL_THRESHOLD_LOW)  seen_freefall = 1; 
                if (total_gyro  > GYRO_THRESHOLD)       seen_rotation = 1; 
                
                if (current_sound > (bg_sound_max + 600)) seen_loud_noise = 1;

                if (seen_impact && (seen_freefall || seen_rotation)) {
                    if (seen_loud_noise) {
                        current_state = STATE_CONFIRMED;
                        // Sentinel + ASCII Art prints ONCE right here
                        sprintf(buffer, 
                            "\r\n___SEND_TELEGRAM_ALERT___\r\n"
                            "!!! CRASH DETECTED - IMMEDIATE ALARM !!!\r\n"
                            "  AAA  L       AAA  RRRR  M   M \r\n"
                            " A   A L      A   A R   R MM MM \r\n"
                            " AAAAA L      AAAAA RRRR  M M M \r\n"
                            " A   A L      A   A R   R M   M \r\n"
                            " A   A LLLLLL A   A R   R M   M \r\n");
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                    } 
                    else {
                        current_state = STATE_STILLNESS_CHECK;
                        state_timer = HAL_GetTick();
                        last_printed_second = -1;
                        sprintf(buffer,
                            "\r\n=================================\r\n"
                            " SSSSS TTTTT  I  L       L     N   N EEEEE SSSSS SSSSS \r\n"
                            " S       T    I  L       L     NN  N E     S     S     \r\n"
                            " SSSSS   T    I  L       L     N N N EEEEE SSSSS SSSSS \r\n"
                            "     S   T    I  L       L     N  NN E         S     S \r\n"
                            " SSSSS   T    I  LLLLLLL LLLLL N   N EEEEE SSSSS SSSSS \r\n"
                            "=================================\r\n"
                            "Silent Fall. Waiting 5s for Recovery...\r\n");
                        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                    }
                }
                else if (HAL_GetTick() - state_timer > 1500) {
                    current_state = STATE_NORMAL;
                    sprintf(buffer, "\r\n--- TIMEOUT (1.5s) - INSUFFICIENT EVIDENCE ---\r\n");
                    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                }
                break;

            case STATE_STILLNESS_CHECK:
                delay_ms = 20; 

                uint32_t elapsed_time = HAL_GetTick() - state_timer;
                int current_second = elapsed_time / 1000;

                if (current_second != last_printed_second && current_second < 5) {
                    char* number_art = "";
                    switch(current_second) {
                        case 0: number_art = "\r\n 55555 \r\n 5     \r\n 5555  \r\n     5 \r\n  555  \r\n"; break;
                        case 1: number_art = "\r\n 4   4 \r\n 4   4 \r\n 44444 \r\n     4 \r\n     4 \r\n"; break;
                        case 2: number_art = "\r\n  333  \r\n 3   3 \r\n   33  \r\n 3   3 \r\n  333  \r\n"; break;
                        case 3: number_art = "\r\n  222  \r\n 2   2 \r\n   22  \r\n  2    \r\n 22222 \r\n"; break;
                        case 4: number_art = "\r\n   1   \r\n  11   \r\n   1   \r\n   1   \r\n  111  \r\n"; break;
                    }
                    sprintf(buffer, "%s", number_art);
                    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                    last_printed_second = current_second;
                }

                if (elapsed_time < 2000 && current_sound > (bg_sound_max + 600)) {
                    current_state = STATE_CONFIRMED;
                    // Sentinel + ASCII Art prints ONCE right here
                    sprintf(buffer, 
                        "\r\n___SEND_TELEGRAM_ALERT___\r\n"
                        "!!! DELAYED CRASH DETECTED - IMMEDIATE ALARM !!!\r\n"
                        "  AAA  L       AAA  RRRR  M   M \r\n"
                        " A   A L      A   A R   R MM MM \r\n"
                        " AAAAA L      AAAAA RRRR  M M M \r\n"
                        " A   A L      A   A R   R M   M \r\n"
                        " A   A LLLLLL A   A R   R M   M \r\n");
                    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                }

                else if (elapsed_time > 2000 && (fabs(total_accel - 9.8f) > 4.5f)) {
                    current_state = STATE_NORMAL;
                    last_printed_second = -1; 
                    sprintf(buffer, 
                        "\r\n=================================\r\n"
                        " N   N  OOO  RRRR  M   M  AAA  L     \r\n"
                        " NN  N O   O R   R MM MM A   A L     \r\n"
                        " N N N O   O RRRR  M M M AAAAA L     \r\n"
                        " N  NN O   O R   R M   M A   A L     \r\n"
                        " N   N  OOO  R   R M   M A   A LLLLL \r\n"
                        "=================================\r\n"
                        "RECOVERY DETECTED (Push/Stand: %.2f)\r\n", fabs(total_accel - 9.8f));
                    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                }
                else if (elapsed_time > 5000) {
                    current_state = STATE_CONFIRMED;
                    last_printed_second = -1; 
                    // Sentinel + ASCII Art prints ONCE right here
                    sprintf(buffer, 
                        "\r\n___SEND_TELEGRAM_ALERT___\r\n"
                        "  AAA  L       AAA  RRRR  M   M \r\n"
                        " A   A L      A   A R   R MM MM \r\n"
                        " AAAAA L      AAAAA RRRR  M M M \r\n"
                        " A   A L      A   A R   R M   M \r\n"
                        " A   A LLLLLL A   A R   R M   M \r\n");
                    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
                }
                break;

            case STATE_CONFIRMED:
                delay_ms = 100; 
                BSP_LED_Toggle(LED2); 
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3); 
                // TOTAL UART SILENCE. No printing allowed here.
                break;
        }

        if (current_state != STATE_CONFIRMED) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 
        }
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
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        while(1); 
    }

    sConfig.Channel = ADC_CHANNEL_13;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        while(1); 
    }
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

int mov_avg_C(int N, int* accel_buff)
{ 
    int result=0;
    for(int i=0; i<N;i++)
    {
        result+=accel_buff[i];
    }
    result=result/4;
    return result;
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
        if (HAL_UART_Init(&huart1) != HAL_OK)
        {
          while(1);
        }
}

static void Buzzer_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void Button_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; 
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

int _read(int file, char *ptr, int len) { return 0; }
int _fstat(int file, struct stat *st) { return 0; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _isatty(int file) { return 1; }
int _close(int file) { return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { return -1; }
  /******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * (c) CG2028 Teaching Team
  ******************************************************************************/


/*--------------------------- Includes ---------------------------------------*/
#include "main.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_accelero.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_tsensor.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_gyro.h"

#include "stdio.h"
#include "string.h"
#include <sys/stat.h>
#include <math.h>

static void UART1_Init(void);

extern void initialise_monitor_handles(void);	// for semi-hosting support (printf). Will not be required if transmitting via UART

extern int mov_avg(int N, int* accel_buff); // asm implementation

int mov_avg_C(int N, int* accel_buff); // Reference C implementation

UART_HandleTypeDef huart1;

// 4-Stage Fall Detection State Machine
typedef enum {
    STATE_NORMAL,
    STATE_FREEFALL,
    STATE_TUMBLE,
    STATE_ALARM
} FallState;


int main(void)
{
	const int N=4;

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* UART initialization  */
	UART1_Init();

	/* Peripheral initializations using BSP functions */
	BSP_LED_Init(LED2);
	BSP_ACCELERO_Init();
	BSP_GYRO_Init();

	/*Set the initial LED state to off*/
	BSP_LED_Off(LED2);

	int accel_buff_x[4]={0};
	int accel_buff_y[4]={0};
	int accel_buff_z[4]={0};
	int i=0;
	
	uint32_t last_led_toggle = 0;
	uint32_t last_uart_transmit = 0;
	uint32_t led_blink_interval = 1000; // default 1 second slow blink

	FallState current_fall_state = STATE_NORMAL;
	uint32_t freefall_start_time = 0;
	uint32_t alarm_start_time = 0;

	while (1)
	{
		uint32_t current_tick = HAL_GetTick();

		// Asynchronous LED Toggling (Does not block sensor reads)
		if (current_tick - last_led_toggle >= led_blink_interval) {
			BSP_LED_Toggle(LED2);
			last_led_toggle = current_tick;
		}

		int16_t accel_data_i16[3] = { 0 };			// array to store the x, y and z readings of accelerometer
		/********Function call to read accelerometer values*********/
		BSP_ACCELERO_AccGetXYZ(accel_data_i16);

		//Copy the values over to a circular style buffer
		accel_buff_x[i%4]=accel_data_i16[0]; //acceleration along X-Axis
		accel_buff_y[i%4]=accel_data_i16[1]; //acceleration along Y-Axis
		accel_buff_z[i%4]=accel_data_i16[2]; //acceleration along Z-Axis


		// ********* Read gyroscope values *********/
		float gyro_data[3]={0.0};
		float* ptr_gyro=gyro_data;
		BSP_GYRO_GetXYZ(ptr_gyro);

		//The output of gyro has been made to display in dps(degree per second)
		float gyro_velocity[3]={0.0};
		gyro_velocity[0] = (gyro_data[0] / 1000.0f);
		gyro_velocity[1] = (gyro_data[1] / 1000.0f);
		gyro_velocity[2] = (gyro_data[2] / 1000.0f);


		//Preprocessing the filtered outputs  The same needs to be done for the output from the C program as well
		float accel_filt_asm[3]={0}; // final value of filtered acceleration values

		accel_filt_asm[0]= (float)mov_avg(N,accel_buff_x) * (9.8/1000.0f);
		accel_filt_asm[1]= (float)mov_avg(N,accel_buff_y) * (9.8/1000.0f);
		accel_filt_asm[2]= (float)mov_avg(N,accel_buff_z) * (9.8/1000.0f);


		//Preprocessing the filtered outputs  The same needs to be done for the output from the assembly program as well
		float accel_filt_c[3]={0};

		accel_filt_c[0]=(float)mov_avg_C(N,accel_buff_x) * (9.8/1000.0f);
		accel_filt_c[1]=(float)mov_avg_C(N,accel_buff_y) * (9.8/1000.0f);
		accel_filt_c[2]=(float)mov_avg_C(N,accel_buff_z) * (9.8/1000.0f);

		/***************************UART transmission*******************************************/
		char buffer[150]; // Create a buffer large enough to hold the text

		/******Transmitting results of C execution over UART*********/
		// Transmit results over UART, limited to twice a second (500ms) to prevent console spam
		if(i>=3 && (current_tick - last_uart_transmit >= 500))
		{
			// 1. First printf() Equivalent
			sprintf(buffer, "Results of C execution for filtered accelerometer readings:\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

			// 2. Second printf() (with Floats) Equivalent
			// Note: Requires -u _printf_float to be enabled in Linker settings
			sprintf(buffer, "Averaged X : %f; Averaged Y : %f; Averaged Z : %f;\r\n",
					accel_filt_c[0], accel_filt_c[1], accel_filt_c[2]);
			HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

			/******Transmitting results of asm execution over UART*********/

			// 1. First printf() Equivalent
			sprintf(buffer, "Results of assembly execution for filtered accelerometer readings:\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

			// 2. Second printf() (with Floats) Equivalent
			// Note: Requires -u _printf_float to be enabled in Linker settings
			sprintf(buffer, "Averaged X : %f; Averaged Y : %f; Averaged Z : %f;\r\n",
					accel_filt_asm[0], accel_filt_asm[1], accel_filt_asm[2]);
			HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

			/******Transmitting Gyroscope readings over UART*********/

			// 1. First printf() Equivalent
			sprintf(buffer, "Gyroscope sensor readings:\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);

			// 2. Second printf() (with Floats) Equivalent
			// Note: Requires -u _printf_float to be enabled in Linker settings
			sprintf(buffer, "Averaged X : %f; Averaged Y : %f; Averaged Z : %f;\r\n\n",
					gyro_velocity[0], gyro_velocity[1], gyro_velocity[2]);
			HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
			
			last_uart_transmit = current_tick;
		}

		// ********* 4-Stage Fall Detection (Physics Engine) *********/
		// Calculate total acceleration magnitude (Vectors sum)
		float total_accel = sqrt(pow(accel_filt_asm[0], 2) + pow(accel_filt_asm[1], 2) + pow(accel_filt_asm[2], 2));

		// Calculate total gyroscope velocity magnitude
		float total_gyro = sqrt(pow(gyro_velocity[0], 2) + pow(gyro_velocity[1], 2) + pow(gyro_velocity[2], 2));

		switch(current_fall_state) {
			case STATE_NORMAL:
				// NORMAL -> FREEFALL
				// The person has visibly started dropping (lost footing/fainting). 
				// Gravity force towards the board drops below 0.7G (7.0 m/s2)
				if (total_accel < 7.0f) {
					current_fall_state = STATE_FREEFALL;
					freefall_start_time = current_tick;
					
					sprintf(buffer, "\r\n[FSM] 1/4: DROP DETECTED! (Accel: %.1f m/s2)\r\n", total_accel);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
				}
				led_blink_interval = 1000; // Normal activity - Blink LED slow (0.5Hz toggle)
				break;

			case STATE_FREEFALL:
				// FREEFALL -> TUMBLE
				// While falling, the human body naturally tips or flails slightly.
				// We look for a gentle rotation (> 50 dps) to confirm it is a human fall, not an elevator or jump.
				if (current_tick - freefall_start_time > 1500) {
					// Timed out (1.5 seconds) - Was just a gentle bump or jump
					current_fall_state = STATE_NORMAL;
					sprintf(buffer, "\r\n[FSM] <-- FALSE ALARM. (No tumble/impact occurred)\r\n");
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
				} 
				else if (total_gyro > 50.0f) { // Caught the "Slight Turn"
					current_fall_state = STATE_TUMBLE;
					
					sprintf(buffer, "\r\n[FSM] 2/4: UNCONTROLLED TUMBLE DETECTED! (Gyro: %.1f dps)\r\n", total_gyro);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
				}
				break;

			case STATE_TUMBLE:
				// TUMBLE -> IMPACT
				// They are now officially falling and rotating. Waiting for the massive deceleration spike.
				if (current_tick - freefall_start_time > 2000) {
					// Timed out (2.0 seconds) - Somehow they never hit the ground?
					current_fall_state = STATE_NORMAL;
					sprintf(buffer, "\r\n[FSM] <-- FALSE ALARM. (They recovered their balance)\r\n");
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
				}
				else if (total_accel > 15.0f) { // > 1.5G Heavy Deceleration Impact
					current_fall_state = STATE_ALARM;
					alarm_start_time = current_tick;
					
					// Instantly fire a one-time critical UART alert overriding the 500ms block
					sprintf(buffer, "\r\n[FSM] 3/4: !!! CRITICAL GROUND IMPACT !!! \r\n");
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
					sprintf(buffer, ">>> Impact Force: %.2f m/s2 | Rotation: %.2f dps <<<\r\n\n", total_accel, total_gyro);
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
				}
				break;

			case STATE_ALARM:
				// [FSM] 4/4: SETTLED & ALARMING
				// Fall detected! Lock the LED into a fast strobe (10Hz toggle)
				led_blink_interval = 100; 
				
				// Hold the screaming alarm state for 5 seconds
				if (current_tick - alarm_start_time > 5000) {
					current_fall_state = STATE_NORMAL; // Automatically reset to normal
					
					sprintf(buffer, "\r\n[FSM] User Rescued. Resetting System.\r\n");
					HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
				}
				break;
		}
		
		i++;
		HAL_Delay(20);	// Sample sensors extremely fast at 50Hz
	}


}



int mov_avg_C(int N, int* accel_buff)
{ 	// The implementation below is inefficient and meant only for verifying your results.
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
        /* Pin configuration for UART. BSP_COM_Init() can do this automatically */
        __HAL_RCC_GPIOB_CLK_ENABLE();
         __HAL_RCC_USART1_CLK_ENABLE();

        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Configuring UART1 */
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


// Do not modify these lines of code. They are written to supress UART related warnings
int _read(int file, char *ptr, int len) { return 0; }
int _fstat(int file, struct stat *st) { return 0; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _isatty(int file) { return 1; }
int _close(int file) { return -1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { return -1; }


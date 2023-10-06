/* U_PiCalc_HS2023.c
 *
 * Created: 3.10.2023
 * Author : Marco Müller
 */

// ===============================
// Standard and AVR includes
// ===============================
#include "math.h"
#include "stdio.h"
#include "string.h"
#include "sleepConfig.h"
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "port_driver.h"
#include "init.h"
#include "utils.h"

// ===============================
// FreeRTOS includes
// ===============================
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"
#include "mem_check.h"
#include "semphr.h"

// ===============================
// Project-specific includes
// ===============================
#include "errorHandler.h"
#include "NHD0420Driver.h"
#include "ButtonHandler.h"

// ===============================
// Function Declarations
// ===============================
extern void vApplicationIdleHook(void);
void vControllerTask(void* pvParameters);
void vPiCalcLeibnizTask(void* pvParameters);
void vPiCalcNilkanthaTask(void* pvParameters);
void vButtonHandler(void* pvParameters);

// ===============================
// Button event bit definitions
// ===============================
#define EVBUTTONS_S1    1<<0
#define EVBUTTONS_S2    1<<1
#define EVBUTTONS_S3    1<<2
#define EVBUTTONS_S4    1<<3
#define EVBUTTONS_CLEAR 0xFF

// ===============================
// Enumerations
// ===============================
typedef enum {
    LEIBNIZ,
    NILKANTHA
} AlgorithmMode;

// ===============================
// Global Variables
// ===============================
// Shared variables to hold the approximation of Pi (for display or analysis)
volatile float pi_approximation_leibniz = 0.0;
volatile float pi_approximation_nilkantha = 3.0;  // Nilkantha starts at 3

// Variables for PI accuracy tracking
volatile BaseType_t piAccuracyAchievedLeibniz = pdFALSE;
volatile BaseType_t piAccuracyAchievedNilkantha = pdFALSE;

// Time tracking variables
TickType_t startTimeLeibniz = 0, elapsedTimeLeibniz = 0;
TickType_t startTimeNilkantha = 0, elapsedTimeNilkantha = 0;

// Current algorithm mode (default is Leibniz)
AlgorithmMode currentAlgorithm = LEIBNIZ;

// Semaphores and Event Groups
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xStartSemaphore = NULL;
SemaphoreHandle_t xStopSemaphore = NULL;
EventGroupHandle_t evButtonEvents;  // Handle for button event group

// ===============================
// Function Definitions
// ===============================

// Idle task hook for FreeRTOS
void vApplicationIdleHook(void) {
    // Currently empty
}

// Main function
int main(void) {
    vInitClock();   // Initialize system clock
    vInitDisplay(); // Initialize display

    // Create event group for button events and semaphores
    evButtonEvents = xEventGroupCreate();
    xResetSemaphore = xSemaphoreCreateBinary();
    xStartSemaphore = xSemaphoreCreateBinary();
    xStopSemaphore = xSemaphoreCreateBinary();

    // Create FreeRTOS tasks with optimized stack size and priority
    xTaskCreate(vButtonHandler, "btTask", configMINIMAL_STACK_SIZE + 50, NULL, 4, NULL);  
    xTaskCreate(vControllerTask, "control_tsk", configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL); 
    xTaskCreate(vPiCalcLeibnizTask, "pi_calc_leibniz", configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL);  
    xTaskCreate(vPiCalcNilkanthaTask, "pi_calc_nilkantha", configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL); 

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    return 0;
}

// Task for calculating pi using the Leibniz formula
void vPiCalcLeibnizTask(void* pvParameters)
{
	uint16_t iterations = 0;
	float sign = 1.0;

	for (;;)
	{
		if(xSemaphoreTake(xStartSemaphore, portMAX_DELAY) == pdTRUE) {
			startTimeLeibniz = xTaskGetTickCount();  // Capture the start time for Leibniz
			while(xSemaphoreTake(xStopSemaphore, 0) == pdFALSE) {
				if(xSemaphoreTake(xResetSemaphore, 0) == pdTRUE) {
					pi_approximation_leibniz = 0.0;
					iterations = 0;
					sign = 1.0;
				}

				pi_approximation_leibniz += (sign / (2 * iterations + 1)) * 4;
				sign = -sign;
				iterations++;
				vTaskDelay(pdMS_TO_TICKS(10));

				// Check accuracy for Leibniz
				if (!piAccuracyAchievedLeibniz && fabs(pi_approximation_leibniz - M_PI) < 0.00001) {
					piAccuracyAchievedLeibniz = pdTRUE;
				}
			}
			elapsedTimeLeibniz = xTaskGetTickCount() - startTimeLeibniz;  // Capture the elapsed time once stopped
		}
	}
}

// Task for calculating pi using the Nilkantha formula
void vPiCalcNilkanthaTask(void* pvParameters)
{
	float n = 2.0;
	float sign = 1.0;

	for (;;)
	{
		if(xSemaphoreTake(xStartSemaphore, portMAX_DELAY) == pdTRUE) {
			startTimeNilkantha = xTaskGetTickCount();  // Capture the start time for Nilkantha
			while(xSemaphoreTake(xStopSemaphore, 0) == pdFALSE) {
				if(xSemaphoreTake(xResetSemaphore, 0) == pdTRUE) {
					pi_approximation_nilkantha = 3.0;
					n = 2.0;
					sign = 1.0;
				}

				pi_approximation_nilkantha += sign * (4 / (n * (n + 1) * (n + 2)));
				sign = -sign;
				n += 2;
				vTaskDelay(pdMS_TO_TICKS(10));

				// Check accuracy for Nilkantha
				if (!piAccuracyAchievedNilkantha && fabs(pi_approximation_nilkantha - M_PI) < 0.00001) {
					piAccuracyAchievedNilkantha = pdTRUE;
				}
			}
			elapsedTimeNilkantha = xTaskGetTickCount() - startTimeNilkantha;  // Capture the elapsed time once stopped
		}
	}
}

// Task for handling display based on button presses
void vControllerTask(void* pvParameters)
{
	for (;;)
	{
		uint32_t buttonState = (xEventGroupGetBits(evButtonEvents)) & 0x000000FF; // Read Button States from EventGroup
		xEventGroupClearBits(evButtonEvents, EVBUTTONS_CLEAR);  // Clear all button event bits

		switch (buttonState)
		{
			case EVBUTTONS_S1: // Start
			xSemaphoreGive(xStartSemaphore);
			break;

			case EVBUTTONS_S2: // Stop
			xSemaphoreGive(xStopSemaphore);
			break;
			
			case EVBUTTONS_S3: // Reset
			xSemaphoreGive(xResetSemaphore);
			break;

			case EVBUTTONS_S4: // Change Algorithm
			currentAlgorithm = (currentAlgorithm == LEIBNIZ) ? NILKANTHA : LEIBNIZ; // Toggle algorithm
			break;

			default:
			break;
		}
		
		if (!piAccuracyAchievedLeibniz) {
			elapsedTimeLeibniz = xTaskGetTickCount() - startTimeLeibniz;
		}
		if (!piAccuracyAchievedNilkantha) {
			elapsedTimeNilkantha = xTaskGetTickCount() - startTimeNilkantha;
		}

		// Display current algorithm's approximation of pi
		if (currentAlgorithm == LEIBNIZ)
		{
			char pistringLeibniz[20];
			char timeStringLeibniz[20];
			
			vDisplayClear();
			vDisplayWriteStringAtPos(0, 0, "Leibniz Series");
			sprintf(pistringLeibniz, "PI: %.8f", pi_approximation_leibniz);
			vDisplayWriteStringAtPos(1, 0, "%s", pistringLeibniz);
			
			sprintf(timeStringLeibniz, "Time: %lu ms", elapsedTimeLeibniz);
			vDisplayWriteStringAtPos(2, 0, "%s", timeStringLeibniz);
			
			vDisplayWriteStringAtPos(3, 0, "#STR #STP #RST #CALG");
		}

		if (currentAlgorithm == NILKANTHA)
		{
			char pistringNilkantha[20];
			char timeStringNilkantha[20];
			
			vDisplayClear();
			vDisplayWriteStringAtPos(0, 0, "Nilkantha Method");
			sprintf(pistringNilkantha, "PI: %.8f", pi_approximation_nilkantha);
			vDisplayWriteStringAtPos(1, 0, "%s", pistringNilkantha);
			
			sprintf(timeStringNilkantha, "Time: %lu ms", elapsedTimeNilkantha);
			vDisplayWriteStringAtPos(2, 0, "%s", timeStringNilkantha);
			
			vDisplayWriteStringAtPos(3, 0, "#STR #STP #RST #CALG");
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// Task for handling button states and setting appropriate event bits
void vButtonHandler(void* pvParameters) {
	initButtons(); // Initialize Button handler
	for(;;) {
		updateButtons(); // Update Button States
		
		// Read Button State and set EventBits in EventGroup based on button press
		if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S1);
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S2);
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S3);
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
			xEventGroupSetBits(evButtonEvents, EVBUTTONS_S4);
		}

		vTaskDelay((1000/BUTTON_UPDATE_FREQUENCY_HZ)/portTICK_RATE_MS);
	}
}
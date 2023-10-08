/* U_PiCalc_HS2023.c
 *
 * Created: 3.10.2023
 * Author : Marco Mueller
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

// Current algorithm mode (default is Leibniz)
AlgorithmMode currentAlgorithm = LEIBNIZ;

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

// Task handling for suspend
TaskHandle_t xLeibnizTaskHandle = NULL;
TaskHandle_t xNilkanthaTaskHandle = NULL;

// Running flag for formula
bool isLeibnizRunning = false;
bool isNilkanthaRunning = false;

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
	xTaskCreate(vPiCalcLeibnizTask, "pi_calc_leibniz", configMINIMAL_STACK_SIZE + 200, NULL, 2, &xLeibnizTaskHandle);
	xTaskCreate(vPiCalcNilkanthaTask, "pi_calc_nilkantha", configMINIMAL_STACK_SIZE + 200, NULL, 2, &xNilkanthaTaskHandle);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    return 0;
}

// Task for calculating pi using the Leibniz formula
void vPiCalcLeibnizTask(void* pvParameters)
{
	uint32_t iterations = 0;
	float sign = 1.0;

	for (;;)
	{
		// Check if we should start the calculation
		if (xSemaphoreTake(xStartSemaphore, 0) == pdTRUE)
		{
			isLeibnizRunning = true;  // Set the flag
			startTimeLeibniz = xTaskGetTickCount(); // Capture start time
		}

		// Check if we should stop the calculation
		if (xSemaphoreTake(xStopSemaphore, 0) == pdTRUE)
		{
			isLeibnizRunning = false;  // Clear the flag
		}

		// Check if we should reset
		if (xSemaphoreTake(xResetSemaphore, 0) == pdTRUE)
		{
			pi_approximation_leibniz = 0.0;
			iterations = 0;
			sign = 1.0;
			isLeibnizRunning = false;  // Clear the flag
			piAccuracyAchievedLeibniz = pdFALSE;
		}

		if (isLeibnizRunning)
		{
			// Leibniz formula for pi approximation
			pi_approximation_leibniz += (sign / (2 * iterations + 1)) * 4;
			
			// Check for accuracy
			if (!piAccuracyAchievedLeibniz && fabs(pi_approximation_leibniz - M_PI) < 0.00001)
			{
				piAccuracyAchievedLeibniz = pdTRUE;
				//isLeibnizRunning = false;  // Optionally, stop the calculation after accuracy is achieved
			}

			sign = -sign;
			iterations++;
		}

		vTaskDelay(pdMS_TO_TICKS(10)); // Optional delay to prevent CPU hogging
	}
}

// Task for calculating pi using the Nilkantha formula
void vPiCalcNilkanthaTask(void* pvParameters)
{
	uint32_t iterations = 0;
	float sign = 1.0;

	for (;;)
	{
		// Check if we should start the calculation
		if (xSemaphoreTake(xStartSemaphore, 0) == pdTRUE)
		{
			isNilkanthaRunning = true; // Set the flag
			startTimeNilkantha = xTaskGetTickCount(); // Capture start time
		}

		// Check if we should stop the calculation
		if (xSemaphoreTake(xStopSemaphore, 0) == pdTRUE)
		{
			isNilkanthaRunning = false; // Set the flag
		}

		// Check if we should reset
		if (xSemaphoreTake(xResetSemaphore, 0) == pdTRUE)
		{
			pi_approximation_nilkantha = 3.0;
			iterations = 0;
			sign = 1.0;
			isNilkanthaRunning = false; // Set the flag
			piAccuracyAchievedNilkantha = pdFALSE;
		}

		if (isNilkanthaRunning)
		{
			// Nilkantha formula for pi approximation
			pi_approximation_nilkantha += sign * (4.0 / ((2 * iterations + 2) * (2 * iterations + 3) * (2 * iterations + 4)));
			
			// Check for accuracy
			if (!piAccuracyAchievedNilkantha && fabs(pi_approximation_nilkantha - M_PI) < 0.00001)
			{
				piAccuracyAchievedNilkantha = pdTRUE;
				elapsedTimeNilkantha = xTaskGetTickCount() - startTimeNilkantha;
				//isRunning = false; // Optionally, stop the calculation after accuracy is achieved
			}

			sign = -sign;
			iterations++;
		}

		vTaskDelay(pdMS_TO_TICKS(10)); // Optional delay to prevent CPU hogging
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
			if(currentAlgorithm == LEIBNIZ) {
				startTimeLeibniz = xTaskGetTickCount(); // Reset start time for Leibniz
				} else if(currentAlgorithm == NILKANTHA) {
				startTimeNilkantha = xTaskGetTickCount(); // Reset start time for Nilkantha
			}
			break;

			case EVBUTTONS_S4: // Change Algorithm
			if(currentAlgorithm == LEIBNIZ) {
				// If currently on Leibniz, switch to Nilkantha
				vTaskSuspend(xLeibnizTaskHandle);
				vTaskResume(xNilkanthaTaskHandle);
				currentAlgorithm = NILKANTHA;
				} else {
				// If currently on Nilkantha, switch to Leibniz
				vTaskSuspend(xNilkanthaTaskHandle);
				vTaskResume(xLeibnizTaskHandle);
				currentAlgorithm = LEIBNIZ;
			}
			break;

			default:
			break;
		}
		
		// Update elapsed time only if the approximation task is running
		if (isLeibnizRunning && !piAccuracyAchievedLeibniz) {
			elapsedTimeLeibniz = xTaskGetTickCount() - startTimeLeibniz;
		}
		if (isNilkanthaRunning && !piAccuracyAchievedNilkantha) {
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
		}
		
		vDisplayWriteStringAtPos(3, 0, "#STR #STP #RST #CALG");

		vTaskDelay(pdMS_TO_TICKS(500));
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
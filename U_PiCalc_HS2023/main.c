/* U_PiCalc_HS2023.c
 *
 * Created: 3.10.2023
 * Author : Marco Müller
 */

// Standard and AVR includes
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

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"
#include "mem_check.h"
#include "semphr.h"

// Project-specific includes
#include "errorHandler.h"
#include "NHD0420Driver.h"
#include "ButtonHandler.h"

extern void vApplicationIdleHook(void);
void vControllerTask(void* pvParameters);
void vPiCalcLeibnizTask(void* pvParameters);
void vPiCalcNilkanthaTask(void* pvParameters);
void vButtonHandler(void* param);

// Button event bit definitions
#define EVBUTTONS_S1	1<<0
#define EVBUTTONS_S2	1<<1
#define EVBUTTONS_S3	1<<2
#define EVBUTTONS_S4	1<<3
#define EVBUTTONS_CLEAR	0xFF

// Shared variable to hold the approximation of Pi (for display or analysis)
volatile float pi_approximation_leibniz = 0.0;
volatile float pi_approximation_nilkantha = 3.0;  // Nilkantha starts at 3

typedef enum {
	LEIBNIZ,
	NILKANTHA
} AlgorithmMode;

AlgorithmMode currentAlgorithm = LEIBNIZ; // Start with Leibniz by default
SemaphoreHandle_t xResetSemaphore = NULL;
EventGroupHandle_t evButtonEvents;  // Handle for button event group

// Idle task hook for FreeRTOS
void vApplicationIdleHook(void)
{
	// Currently empty
}

int main(void)
{
	vInitClock();   // Initialize system clock
	vInitDisplay(); // Initialize display

	// Create event group for button events
	evButtonEvents = xEventGroupCreate();
	xResetSemaphore = xSemaphoreCreateBinary();

	// Create FreeRTOS tasks
	xTaskCreate(vControllerTask, "control_tsk", configMINIMAL_STACK_SIZE + 150, NULL, 3, NULL);
	xTaskCreate(vPiCalcLeibnizTask, "pi_calc_leibniz", configMINIMAL_STACK_SIZE + 150, NULL, 2, NULL);
	xTaskCreate(vPiCalcNilkanthaTask, "pi_calc_nilkantha", configMINIMAL_STACK_SIZE + 150, NULL, 2, NULL);
	xTaskCreate(vButtonHandler, (const char*) "btTask", configMINIMAL_STACK_SIZE+30, NULL, 2, NULL);

	// Start the FreeRTOS scheduler
	vTaskStartScheduler();
	return 0;
}

// Task for calculating pi using the Leibniz formula
void vPiCalcLeibnizTask(void* pvParameters)
{
	uint16_t iterations = 0;  // Keep track of the number of iterations/terms
	float sign = 1.0;   // To alternate between adding and subtracting terms

	for (;;)
	{
		// Check if the semaphore has been given
		if(xSemaphoreTake(xResetSemaphore, 0) == pdTRUE) {
			pi_approximation_leibniz = 0.0;
			iterations = 0;
			sign = 1.0;
		}
		
		pi_approximation_leibniz += (sign / (2 * iterations + 1)) * 4;  // Leibniz formula term
		sign = -sign;  // Alternate sign
		iterations++;
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// Task for calculating pi using the Nilkantha formula
void vPiCalcNilkanthaTask(void* pvParameters)
{
	float n = 2.0;  // Start term
	float sign = 1.0;  // Start with adding

	for (;;)
	{
		if(xSemaphoreTake(xResetSemaphore, 0) == pdTRUE) {
			pi_approximation_nilkantha = 3.0;
			n = 2.0;
			sign = 1.0;
		}
		
		pi_approximation_nilkantha += sign * (4 / (n * (n + 1) * (n + 2)));  // Nilkantha series term
		sign = -sign;  // Alternate sign
		n += 2;
		vTaskDelay(pdMS_TO_TICKS(10));
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
			break;

			case EVBUTTONS_S2: // Stop
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

		// Display current algorithm's approximation of pi
		if (currentAlgorithm == LEIBNIZ)
		{
			char pistringLeibniz[20];
			// Display the Leibniz approximation of pi
			vDisplayClear();
			vDisplayWriteStringAtPos(0, 0, "Leibniz");
			sprintf(pistringLeibniz, "PI: %.8f", pi_approximation_leibniz);
			vDisplayWriteStringAtPos(1, 0, "%s", pistringLeibniz);
			vDisplayWriteStringAtPos(3, 0, "#STR #STP #RST #CALG");
		}
		else
		{
			char pistringNilkantha[20];
			// Display the Nilkantha approximation of pi
			vDisplayClear();
			vDisplayWriteStringAtPos(0, 0, "Nilkantha");
			sprintf(pistringNilkantha, "PI: %.8f", pi_approximation_nilkantha);
			vDisplayWriteStringAtPos(1, 0, "%s", pistringNilkantha);
			vDisplayWriteStringAtPos(3, 0, "#STR #STP #RST #CALG");
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// Task for handling button states and setting appropriate event bits
void vButtonHandler(void* param) {
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

		// Delay to match the button update frequency
		vTaskDelay((1000/BUTTON_UPDATE_FREQUENCY_HZ)/portTICK_RATE_MS);
	}
}
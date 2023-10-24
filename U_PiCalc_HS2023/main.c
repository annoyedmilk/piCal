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

// Function Declarations
extern void vApplicationIdleHook(void);
void vControllerTask(void* pvParameters);
void vPiCalcTask(void* pvParameters);
void vButtonHandler(void* pvParameters);

// Button event bit definitions using enum for clarity
typedef enum {
	EVBUTTONS_S1 = 1<<0,
	EVBUTTONS_S2 = 1<<1,
	EVBUTTONS_S3 = 1<<2,
	EVBUTTONS_S4 = 1<<3,
	EVBUTTONS_CLEAR = 0xFF
} ButtonEvents;

typedef enum {
	LEIBNIZ,
	NILKANTHA
} AlgorithmMode;

AlgorithmMode currentAlgorithm = LEIBNIZ;

// Global Variables
volatile float pi_approximation = 0.0;
volatile BaseType_t piAccuracyAchieved = pdFALSE;
TickType_t startTime = 0, elapsedTime = 0;
bool isRunning = false;
TaskHandle_t xPiCalcTaskHandle = NULL;
SemaphoreHandle_t xResetSemaphore = NULL;
SemaphoreHandle_t xStartSemaphore = NULL;
SemaphoreHandle_t xStopSemaphore = NULL;
EventGroupHandle_t evButtonEvents;

void vApplicationIdleHook(void) {}

int main(void) {
	vInitClock();
	vInitDisplay();

	evButtonEvents = xEventGroupCreate();
	xResetSemaphore = xSemaphoreCreateBinary();
	xStartSemaphore = xSemaphoreCreateBinary();
	xStopSemaphore = xSemaphoreCreateBinary();

	xTaskCreate(vButtonHandler, "btTask", configMINIMAL_STACK_SIZE + 50, NULL, 4, NULL);
	xTaskCreate(vControllerTask, "control_tsk", configMINIMAL_STACK_SIZE + 100, NULL, 3, NULL);
	xTaskCreate(vPiCalcTask, "pi_calc", configMINIMAL_STACK_SIZE + 200, NULL, 2, &xPiCalcTaskHandle);

	vTaskStartScheduler();
	return 0;
}

void vPiCalcTask(void* pvParameters) {
	uint32_t iterations = 0;
	float sign = 1.0;

	for (;;) {
		if (xSemaphoreTake(xStartSemaphore, 0) == pdTRUE) {
			isRunning = true;
			startTime = xTaskGetTickCount();
		}

		if (xSemaphoreTake(xStopSemaphore, 0) == pdTRUE) {
			isRunning = false;
		}

		if (xSemaphoreTake(xResetSemaphore, 0) == pdTRUE) {
			pi_approximation = (currentAlgorithm == LEIBNIZ) ? 0.0 : 3.0;
			iterations = 0;
			sign = 1.0;
			isRunning = false;
			piAccuracyAchieved = pdFALSE;
		}

		if (isRunning) {
			if (currentAlgorithm == LEIBNIZ) {
				pi_approximation += (sign / (2 * iterations + 1)) * 4;
				} else {
				pi_approximation += sign * (4.0 / ((2 * iterations + 2) * (2 * iterations + 3) * (2 * iterations + 4)));
			}

			if (!piAccuracyAchieved && fabs(pi_approximation - M_PI) < 0.00001) {
				piAccuracyAchieved = pdTRUE;
				elapsedTime = xTaskGetTickCount() - startTime;
			}

			sign = -sign;
			iterations++;
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void vControllerTask(void* pvParameters) {
	for (;;) {
		uint32_t buttonState = (xEventGroupGetBits(evButtonEvents)) & 0x000000FF;
		xEventGroupClearBits(evButtonEvents, EVBUTTONS_CLEAR);

		switch (buttonState) {
			case EVBUTTONS_S1:
			xSemaphoreGive(xStartSemaphore);
			break;

			case EVBUTTONS_S2:
			xSemaphoreGive(xStopSemaphore);
			break;

			case EVBUTTONS_S3:
			xSemaphoreGive(xResetSemaphore);
			startTime = xTaskGetTickCount();
			break;

			case EVBUTTONS_S4:
			if (currentAlgorithm == LEIBNIZ) {
				currentAlgorithm = NILKANTHA;
				} else {
				currentAlgorithm = LEIBNIZ;
			}
			xSemaphoreGive(xResetSemaphore);
			break;

			default:
			break;
		}

		if (isRunning && !piAccuracyAchieved) {
			elapsedTime = xTaskGetTickCount() - startTime;
		}

		char piString[20];
		char timeString[20];
		char* method = (currentAlgorithm == LEIBNIZ) ? "Leibniz Series" : "Nilkantha Method";

		vDisplayClear();
		vDisplayWriteStringAtPos(0, 0, method);
		sprintf(piString, "PI: %.8f", pi_approximation);
		vDisplayWriteStringAtPos(1, 0, "%s", piString);
		sprintf(timeString, "Time: %lu ms", elapsedTime);
		vDisplayWriteStringAtPos(2, 0, "%s", timeString);
		vDisplayWriteStringAtPos(3, 0, "#STR #STP #RST #CALG");

		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void vButtonHandler(void* pvParameters) {
	initButtons();
	for(;;) {
		updateButtons();
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
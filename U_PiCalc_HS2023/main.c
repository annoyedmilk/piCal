/* U_PiCalc_HS2023.c
 *
 * Created: 3.10.2023
 * Author : Marco Müller
 */

#include "math.h"
#include "stdio.h"
#include "string.h"
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"
#include "mem_check.h"
#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"
#include "ButtonHandler.h"

void controllerTask(void* pvParameters);
void PiCalcLeibnizTask(void* pvParameters);
void PiCalcNilkanthaTask(void* pvParameters);

// Shared variable to hold the approximation of Pi (for display or analysis)
volatile float pi_approximation_leibniz = 0.0;
volatile float pi_approximation_nilkantha = 3.0;  // Nilkantha starts at 3

int main(void)
{
	vInitClock();
	vInitDisplay();

	xTaskCreate(controllerTask, "control_tsk", configMINIMAL_STACK_SIZE + 150, NULL, 3, NULL);
	xTaskCreate(PiCalcLeibnizTask, "pi_calc_leibniz", configMINIMAL_STACK_SIZE + 150, NULL, 2, NULL);
	xTaskCreate(PiCalcNilkanthaTask, "pi_calc_nilkantha", configMINIMAL_STACK_SIZE + 150, NULL, 2, NULL);

	vDisplayClear();
	vDisplayWriteStringAtPos(0, 0, "PI-Calculator");

	vTaskStartScheduler();
	return 0;
}

void PiCalcLeibnizTask(void* pvParameters)
{
	int iterations = 0;  // Keep track of the number of iterations/terms
	float sign = 1.0;   // To alternate between adding and subtracting terms

	while (1)
	{
		pi_approximation_leibniz += (sign / (2 * iterations + 1)) * 4;  // Leibniz formula term
		sign = -sign;  // Alternate sign
		iterations++;
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// Nilkantha Task for Calculating Pi
void PiCalcNilkanthaTask(void* pvParameters)
{
	float n = 2.0;  // Start term
	float sign = 1.0;  // Start with adding

	while (1)
	{
		pi_approximation_nilkantha += sign * (4 / (n * (n + 1) * (n + 2)));  // Nilkantha series term
		sign = -sign;  // Alternate sign
		n += 2;
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void controllerTask(void* pvParameters)
{
	initButtons();

	for (;;)
	{
		updateButtons();

		// Handle button presses
		for (int button = BUTTON1; button <= BUTTON4; ++button)
		{
			int pressType = getButtonPress(button);

			switch (pressType)
			{
				case SHORT_PRESSED:
				if (button == BUTTON1)
				{
					char pistringLeibniz[20];
					char pistringNilkantha[20];

					// Display the Leibniz approximation of pi
					sprintf(pistringLeibniz, "L PI: %.8f", pi_approximation_leibniz);
					vDisplayWriteStringAtPos(1, 0, "%s", pistringLeibniz);

					// Display the Nilkantha approximation of pi
					sprintf(pistringNilkantha, "N PI: %.8f", pi_approximation_nilkantha);
					vDisplayWriteStringAtPos(2, 0, "%s", pistringNilkantha);
				}
				break;

				case LONG_PRESSED:
				// Handle long press for each button if needed
				break;

				default:
				break;
			}
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
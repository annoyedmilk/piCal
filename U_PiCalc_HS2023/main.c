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

int main(void)
{
	vInitClock();
	vInitDisplay();

	xTaskCreate(controllerTask, "control_tsk", configMINIMAL_STACK_SIZE + 150, NULL, 3, NULL);

	vDisplayClear();
	vDisplayWriteStringAtPos(0, 0, "PI-Calc");

	vTaskStartScheduler();
	return 0;
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
				// Handle short press
				if (button == BUTTON1)
				{
					char pistring[12];
					sprintf(pistring, "PI: %.8f", M_PI);
					vDisplayWriteStringAtPos(1, 0, "%s", pistring);
				}
				// Handle other buttons if needed
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
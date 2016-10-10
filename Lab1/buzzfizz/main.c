// Initial code from James Adams

#include "main.h"
#include "MKL46Z4.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static xQueueHandle xQueue = NULL;

static void vATaskFunction( void *pvParameters)
{
	portTickType xNextWakeTime;

	unsigned long i = 0;
	while(1)
	{
		++i;
		vTaskDelay(100/ portTICK_PERIOD_MS);
		xQueueSend( xQueue, &i, 0 );		
	}
}

static void vBTaskFunction( void *pvParameters)
{
	unsigned long ulReceivedValue;
	while(1)
	{
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );
		if((ulReceivedValue%3)==0){
			PTE->PTOR = (1U << 29U);
		}
		if((ulReceivedValue%5)==0){
		 	PTD->PTOR = (1U << 5U);
		}
	}
}

int main(void)
{
	gpio_init();
	PTE -> PSOR |= (1U << 29U);
	PTD -> PSOR |= (1U << 5U);

	xQueue = xQueueCreate( 1, sizeof( unsigned long) );

	if (xQueue != NULL) {
		xTaskCreate( vATaskFunction, (signed char *) "RX",
			configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY +2, NULL );
		xTaskCreate( vBTaskFunction, (signed char *) "TX",
                        configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY +1, NULL );
		vTaskStartScheduler();
	}

	while(1);	
}

void gpio_init(void)
 {
     SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK; // Enable clock to PORTD
     PORTE->PCR[29] = PORT_PCR_MUX(1U); // Set pin 5 of PORTD as GPIO
     PTE->PDDR |= (1U << 29U); // Port direction register

     SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
     PORTD->PCR[5] = PORT_PCR_MUX(1U);
     PTD->PDDR |= (1U << 5U); // Port direction register
}

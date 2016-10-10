/*
 * Copyright (c) 12.2013, Martin Kojtal (0xc0170)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
		 if(((ulReceivedValue%3)||(ulReceivedValue%5))== 0){
	            PTE->PTOR = (1U << 29U);
				PTD->PTOR = (1U << 5U);
		 }else if((ulReceivedValue%3)==0){
			 	PTE->PTOR = (1U << 29U);
		 }else if((ulReceivedValue%5)==0){
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
     SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
     PORTE->PCR[29] = PORT_PCR_MUX(1U);
     PTE->PDDR |= (1U << 29U);

     SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
     PORTD->PCR[5] = PORT_PCR_MUX(1U);
     PTD->PDDR |= (1U << 5U);
}

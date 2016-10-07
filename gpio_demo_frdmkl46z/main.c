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

int main(void)
{
    gpio_init();

    PTE->PSOR |= (1U << 29U);
    PTD->PSOR |= (0U << 5U);

    while (1) {
	PTE->PTOR = (1U << 29U);
	PTD->PTOR = (1U << 5U); 
	delay();
	PTE->PTOR = (1U << 29U);
	PTD->PTOR = (1U << 5U); 
	delay();
	delayShort();
    }
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

 void delayShort(void)
 {
     volatile unsigned int i,j;

     for (i = 0U; i < 100U; i++) {
         for (j = 0U; j < 100U; j++) {
             __asm__("nop");
         }
     }

 }
 void delay(void)
 {
     volatile unsigned int i,j;
     for (i = 0U; i < 20000U; i++) {
         for (j = 0U; j < 100U; j++) {
             __asm__("nop");
         }
     }
 }


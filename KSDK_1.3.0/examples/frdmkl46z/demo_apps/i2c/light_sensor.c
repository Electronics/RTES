#include "board.h"

void light_sensor_init(void)
{
		SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    ADC0_CFG1 |= ADC_CFG1_MODE(3);
}

uint16_t light_sensor_read(void){

  ADC0_SC1A = (0b000011);

  while (!(ADC0_SC1A & ~(ADC_SC1_COCO_MASK))){}

  uint16_t data = ADC0_RA;

  return data;
}

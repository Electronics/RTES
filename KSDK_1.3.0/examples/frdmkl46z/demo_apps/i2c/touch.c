#include "board.h"

#define _BV(bit) (1U << bit)

void touch_init() {
  SIM->SCGC5 |= _BV(10); // enable port B
  SIM->SCGC5 |= _BV(5); // Enable clock to TSI

  TSI0_TSHD = 0;
  TSI0_DATA = 0;
  // 16uA 600mV PS:4
  // Channels 9 and 10
  TSI0_GENCS =  _BV(21) | _BV(20) | _BV(16) | _BV(14) | _BV(7);
  //               1uA      600mV    1uA         / 4     enable
}

uint16_t touch_read(uint8_t channel) {
  TSI0_DATA = (channel << 28);
  TSI0_DATA |= _BV(22); // cause a scan
  while(!(TSI0_GENCS & _BV(2))); //wait for scan to complete
  TSI0_GENCS |= _BV(2); // reset flag
  return TSI0_DATA & 0xFF;
}

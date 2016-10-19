#include "board.h"
#include "MKL46Z4.h"
#include "fsl_debug_console.h"
#include "accel_i2c.c"
#include "touch.c"

void PRINTHEX(uint16_t b) {
  char buf[6];
  snprintf(buf,sizeof(buf),"%d",b);
  PRINTF(buf);
  return;
}

int main() {
  hardware_init();
  dbg_uart_init();
  touch_init();
  OSA_Init();

  //write_i2c(ACCEL_WRITE_CMD,ACCEL_CTRL_1,0b00000001); // activate the accel
  PRINTF("Initialised\r\n");

  while(1) {
    OSA_TimeDelay(100);
    uint16_t first = touch_read(9);
    uint16_t second = touch_read(10);

    PRINTF("Touch:");PRINTHEX(first);PRINTF(" ");PRINTHEX(second);PRINTF("\r\n");

  }

  return 1;
}

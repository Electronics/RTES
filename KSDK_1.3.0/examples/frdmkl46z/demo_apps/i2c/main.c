#include "board.h"
#include "MKL46Z4.h"
#include "fsl_debug_console.h"
#include "accel_i2c.c"

int main() {
  hardware_init();
  OSA_Init();

  write_i2c(ACCEL_WRITE_CMD,ACCEL_CTRL_1,0b00000001); // activate the accel

  while(1) {
    //write_i2c(ACCEL_WRITE_CMD,0x2A,0);
    //OSA_TimeDelay(500);
    //write_i2c(ACCEL_WRITE_CMD,0x2A,0);
    OSA_TimeDelay(100);

    read_i2c(ACCEL_WRITE_CMD,ACCEL_READ_CMD,ACCEL_OUT_X_MSB);

    //write_i2c(ACCEL_WRITE_CMD,0x2A,0);

  }

  return 1;
}

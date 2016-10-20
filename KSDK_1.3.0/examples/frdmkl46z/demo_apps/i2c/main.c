#include "board.h"
#include "MKL46Z4.h"
#include "fsl_debug_console.h"
#include "accel_i2c.c"
#include "touch.c"

int main() {
  hardware_init();
  dbg_uart_init();
  touch_init();
  OSA_Init();

  //write_i2c(ACCEL_WRITE_CMD,ACCEL_CTRL_1,0b00000001); // activate the accel
  PRINTF("Initialised\r\n");



  while(1) {
    // Touch sensor calibration
    uint16_t touchCal = 0;
    uint16_t leftTouch, rightTouch;
    for(int i=0;i<100;i++) {
      leftTouch = touch_read(9);
      rightTouch = touch_read(10);

      if(leftTouch>rightTouch+1 || rightTouch>leftTouch+1) {
        PRINTF("ERROR: Finger on sensor during calibration (Levels %d %d)\r\n",leftTouch,rightTouch);
        break;
      } else {
        touchCal+=leftTouch;
      }
    }
    touchCal = touchCal/100;
    PRINTF("Calibration level: %d\r\n",touchCal);
    OSA_TimeDelay(100);
    uint16_t first = touch_read(9);
    uint16_t second = touch_read(10);

    PRINTF("Touch %d %d\r\n",first,second);
    OSA_TimeDelay(1000);

  }

  return 1;
}

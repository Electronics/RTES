#include "board.h"
#include "MKL46Z4.h"
#include "fsl_debug_console.h"
#include "accel_i2c.c"
#include "touch.c"

#define DEADZONE 20
#define MAXCLICK 60

int main() {
  hardware_init();
  dbg_uart_init();
  touch_init();
  OSA_Init();

  //write_i2c(ACCEL_WRITE_CMD,ACCEL_CTRL_1,0b00000001); // activate the accel
  PRINTF("Initialised\r\n");

  uint16_t first;
  uint16_t second;
  uint16_t touchCal = 0;
  int difference;
  bool lClick = 0;
  uint8_t mClick = 0;
  bool rClick = 0;

  while(1) {
    // Touch sensor calibration
    for(int i=0;i<100;i++) {
     first = touch_read(9);
     second = touch_read(10);

     if(first>second+1 || second>first+1) {
       //PRINTF("ERROR: Finger on sensor during calibration (Levels %d %d)\r\n",leftTouch,rightTouch);
       break;
     } else {
       touchCal+=first;
     }
    }
    touchCal = touchCal/100;

    //OSA_TimeDelay(100);
    first = touch_read(9);
    second = touch_read(10);
    difference = first - second;

    //PRINTF("%d %d %d\r\n",first,second,difference);
    if(difference<-DEADZONE) {
      //PRINTF("LMB\r\n");
      lClick = true;
    } else if(difference>DEADZONE) {
      //PRINTF("RMB\r\n");
      rClick = true;
    } if(first>touchCal+1 || touchCal>first+1 || second>touchCal+1 || touchCal>second+1) {
      if(abs(difference)<DEADZONE) {
        //PRINTF("MMB\r\n");
        if(mClick++>MAXCLICK) mClick=MAXCLICK;
      } else {
        if(mClick--<1) mClick=0;
      }
    } else {
      lClick = false;
      if(mClick--<1) mClick=0;
      rClick = false;
    }
    if(lClick) PRINTF("LMB\r\n");
    if(mClick==MAXCLICK) PRINTF("!!!!MMB\r\n");
    if(rClick) PRINTF("RMB\r\n");
    //OSA_TimeDelay(1000);

  }

  return 1;
}

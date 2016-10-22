#include "board.h"
#include "MKL46Z4.h"
#include "fsl_debug_console.h"
#include "accel_i2c.c"
#include "light_sensor.c"
#include "touch.c"

#define DEADZONE 20
#define LIGHTDEADZONE 800
#define MAXCLICK 200

void init_buttons() {
  SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK; // Enable clock
  PORTC->PCR[3] |= PORT_PCR_MUX(1U)|3; // Enable pin + pullup
  PORTC->PCR[12] |= PORT_PCR_MUX(1U)|3;
  PTC->PDDR |= (0U << 3U); // set as Input pins
  PTC->PDDR |= (0U << 12U);
  return;
}

int main() {
  hardware_init();
  dbg_uart_init();
  touch_init();
  light_sensor_init();
  init_buttons();
  LED1_EN;
  LED2_EN;
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
  uint8_t leftClickStatus,rightClickStatus,lastClickStatus=0;
  bool mode=false;
  int lastDifference = 0;
  bool wasTouch=false;
  uint8_t lastClickStatusMode1=0;
  uint16_t lightCalibration = 0;

  // dummy read to start ADC
  light_sensor_read();
  OSA_TimeDelay(1);
  // Get a level for the light sensor (with averaging)
  lightCalibration=(light_sensor_read()+light_sensor_read()+light_sensor_read()+light_sensor_read())>>2;
  PRINTF("%d\r\n",lightCalibration);

  while(1) {
    leftClickStatus = (PTC->PDIR & (1U<<3))>>3;
    rightClickStatus = (PTC->PDIR & (1U<<12))>>12; // These are inversed (pullups)
    if((!leftClickStatus) && (!rightClickStatus)) {
      // Both pressed
      if(!lastClickStatus) {
        lastClickStatus=1;
        mode = !mode;
        lastDifference=0;
        //PRINTF("Changed mode %d\r\n",mode);
        if(mode) LED1_ON;
        else LED1_OFF;
        OSA_TimeDelay(1000);
        continue; // Let us read the buttons etc again
      }
    } else {
      lastClickStatus=0;
    }

    // Check if there's a hand on the mouse
    if(!(light_sensor_read()>lightCalibration+LIGHTDEADZONE)) {
      //PRINTF("No hand %d %d\r\n",lightCalibration,light_sensor_read());
      LED2_ON;
      OSA_TimeDelay(50);
      continue;
    } else {
      //PRINTF("ye hand %d %d\r\n",lightCalibration,light_sensor_read());
      LED2_OFF;
    }

    if(mode) {
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

      if(!leftClickStatus) {
        if(!(lastClickStatusMode1 & 1)) {
          PRINTF("LCLICK DOWN\r\n",lastClickStatusMode1);
          lastClickStatusMode1|=1;
        }
      } else {
        if(lastClickStatusMode1&1) {
          PRINTF("LCLICK UP\r\n");
          lastClickStatusMode1&=~1; // Stops us from clicking multiple times when holding down button
        }
      }
      if(!rightClickStatus) {
        if(!(lastClickStatusMode1&2)) {
          PRINTF("RCLICK DOWN\r\n");
          lastClickStatusMode1|=2;
        }
      } else {
        if(lastClickStatusMode1&2) {
          PRINTF("RCLICK UP\r\n");
          lastClickStatusMode1&=~2; // Stops us from clicking multiple times when holding down button
        }
      }


      //OSA_TimeDelay(100);
      first = touch_read(9);
      second = touch_read(10);
      difference = first - second;

      if(first>touchCal+1 || touchCal>first+1 || second>touchCal+1 || touchCal>second+1) {
        if(!wasTouch) {
          wasTouch=true;
          OSA_TimeDelay(100);
          continue;
        }
        //PRINTF("%d %d %d %d %d\r\n",difference,abs(lastDifference-difference),lastDifference,first,second);
        if(lastDifference==0 || abs(lastDifference-difference)>7) {
          lastDifference=difference; // If we've just entered scroll mode or the user has lifted their finger, reset the lastDifference
          //PRINTF("Reset\r\n");
        }
        if(lastDifference>difference+5) {
          PRINTF("SCROLL UP %d %d\r\n",difference,lastDifference);
          lastDifference=difference;
        } else if(difference>lastDifference+5) {
          PRINTF("SCROLL DOWN %d %d\r\n",difference,lastDifference);
          lastDifference=difference;
        }
      } else {
        //OSA_TimeDelay(10);
        wasTouch=false;
        lastDifference=0; // So we know not to scroll instantly when the user puts their finger back down
      }
      OSA_TimeDelay(1);

    } else {
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

    }
    //OSA_TimeDelay(1000);

  }

  return 1;
}

#include "board.h"
#include "MKL46Z4.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "accel_i2c.c"
#include "light_sensor.c"
#include "touch.c"

struct mouse_struct {
  int x,y,scroll;
  bool leftClickDown,middleClickDown,rightClickDown;
  bool leftClickUp,middleClickUp,rightClickUp;
} mouse;

#define DEADZONE 20 //for touch sensor
#define POS_DEADZONE 20
#define POS_SCALE 5
#define LIGHTDEADZONE 800
#define MAXCLICK 200

#define RINGBUFFER 64

void init_buttons() {
  SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK; // Enable clock
  PORTC->PCR[3] |= PORT_PCR_MUX(1U)|3; // Enable pin + pullup
  PORTC->PCR[12] |= PORT_PCR_MUX(1U)|3;
  PTC->PDDR |= (0U << 3U); // set as Input pins
  PTC->PDDR |= (0U << 12U);
  return;
}

static void prvHardwareMonitorTask(void *pvParameters) {

  uint16_t first;
  uint16_t second;
  uint16_t touchCal = 0;
  int difference;
  bool lClick = false;
  uint8_t mClick = 0; // Allows to check for close-to-middle presses
  bool rClick = false;
  bool mClickBool = false;
  uint8_t leftClickStatus,rightClickStatus,lastClickStatus=0;
  bool mode=false;
  int lastDifference = 0;
  bool wasTouch=false;
  uint8_t lastClickStatusMode1=0;
  uint16_t lightCalibration = 0;
  int8_t xRingBuffer[RINGBUFFER] = {0};
  int8_t yRingBuffer[RINGBUFFER] = {0};
  int8_t zRingBuffer[RINGBUFFER] = {0};
  int32_t mouseX=0;
  int32_t mouseY=0;
  int32_t mouseZ=0;
  int ringPointer=0;

  // dummy read to start ADC
  light_sensor_read();
  OSA_TimeDelay(1);
  // Get a level for the light sensor (with averaging)
  lightCalibration=(light_sensor_read()+light_sensor_read()+light_sensor_read()+light_sensor_read())>>2;
  PRINTF("Light sensor calibration: %d\r\n",lightCalibration);

  while(1) {
    //PRINTF("L: %d %d M: %d %d R: %d %d, X %d Y %d S %d\r\n",mouse.leftClickDown,mouse.leftClickUp,mouse.middleClickDown,mouse.middleClickUp,mouse.rightClickDown,mouse.rightClickUp,mouse.x,mouse.y,mouse.scroll);

    leftClickStatus = (PTC->PDIR & (1U<<3))>>3;
    rightClickStatus = (PTC->PDIR & (1U<<12))>>12; // These are inversed (pullups)
    if((!leftClickStatus) && (!rightClickStatus)) {
      // Both pressed
      if(!lastClickStatus) {
        lastClickStatus=1;
        mode = !mode;
        lastDifference=0;
        PRINTF("Changed mode %d\r\n",mode);
        if(mode) LED1_ON;
        else LED1_OFF;
        mouse.x=0;
        mouse.y=0;
        vTaskDelay(1000/portTICK_RATE_MS);
        continue; // Let us read the buttons etc again
      }
    } else {
      lastClickStatus=0;
    }

    // Check if there's a hand on the mouse
    if(!(light_sensor_read()>lightCalibration+LIGHTDEADZONE)) {
      //PRINTF("No hand %d %d\r\n",lightCalibration,light_sensor_read());
      LED2_ON;
      vTaskDelay(50/portTICK_RATE_MS);
      continue;
    } else {
      //PRINTF("ye hand %d %d\r\n",lightCalibration,light_sensor_read());
      LED2_OFF;
    }
    // if we get here there is a hand on the mouse (probably)

    xRingBuffer[ringPointer] = read_i2c(ACCEL_WRITE_CMD,ACCEL_READ_CMD,ACCEL_OUT_X_MSB);
    vTaskDelay(1/portTICK_RATE_MS); // prevent i2c arbitration
    yRingBuffer[ringPointer] = read_i2c(ACCEL_WRITE_CMD,ACCEL_READ_CMD,ACCEL_OUT_Y_MSB);
    vTaskDelay(1/portTICK_RATE_MS); // prevent i2c arbitration
    zRingBuffer[ringPointer] = read_i2c(ACCEL_WRITE_CMD,ACCEL_READ_CMD,ACCEL_OUT_Z_MSB);
    //xRingBuffer[xRingPointer] |= read_i2c(ACCEL_WRITE_CMD,ACCEL_READ_CMD,ACCEL_OUT_X_LSB);
    //PRINTF("%d %d %d  %d\r\n",xRingBuffer[ringPointer],yRingBuffer[ringPointer],zRingBuffer[ringPointer],ringPointer);
    if(++ringPointer>RINGBUFFER-1) ringPointer=0;

    // Now do the average
    mouseX=0;mouseY=0;mouseZ=0;
    for (int i=0;i<RINGBUFFER;i++) {
      mouseX+=xRingBuffer[i];
      mouseY+=yRingBuffer[i];
      mouseZ+=zRingBuffer[i];
    }
    mouseX=mouseX/RINGBUFFER;
    mouseY=mouseY/RINGBUFFER;
    mouseZ=mouseZ/RINGBUFFER;

    if(abs(mouseX)>POS_DEADZONE) {
      mouse.x=(mouseX-POS_DEADZONE)/POS_SCALE;
    } else {
      mouse.x=0;
    }
    if(abs(mouseY)>POS_DEADZONE) {
      mouse.y=(mouseY-POS_DEADZONE)/POS_SCALE;
    } else {
      mouse.y=0;
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
          //PRINTF("LCLICK DOWN\r\n",lastClickStatusMode1);
          mouse.leftClickDown=true;
          lastClickStatusMode1|=1;
        }
      } else {
        if(lastClickStatusMode1&1) {
          //PRINTF("LCLICK UP\r\n");
          mouse.leftClickUp=true;
          lastClickStatusMode1&=~1; // Stops us from clicking multiple times when holding down button
        }
      }
      if(!rightClickStatus) {
        if(!(lastClickStatusMode1&2)) {
          //PRINTF("RCLICK DOWN\r\n");
          mouse.rightClickDown=true;
          lastClickStatusMode1|=2;
        }
      } else {
        if(lastClickStatusMode1&2) {
          //PRINTF("RCLICK UP\r\n");
          mouse.rightClickUp=true;
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
          //PRINTF("SCROLL UP %d %d\r\n",difference,lastDifference);
          mouse.scroll+=1;
          lastDifference=difference;
        } else if(difference>lastDifference+5) {
          mouse.scroll-=1;
          //PRINTF("SCROLL DOWN %d %d\r\n",difference,lastDifference);
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
        mouse.leftClickDown=true;
        lClick = true; // so we can tell which button was released
      } else if(difference>DEADZONE) {
        //PRINTF("RMB\r\n");
        mouse.rightClickDown=true;
        rClick = true; // so we can tell which button was released
      } if(first>touchCal+1 || touchCal>first+1 || second>touchCal+1 || touchCal>second+1) {
        if(abs(difference)<DEADZONE) {
          //PRINTF("MMB\r\n");
          if(mClick++>MAXCLICK) {
            mClick=MAXCLICK;
            mouse.middleClickDown=true;
            mClickBool=true;
          }
        } else {
          if(mClick--<1) mClick=0;
        }
      } else {
        if(lClick) {
          lClick = false;
          mouse.leftClickUp=true;
        }
        if(mClickBool) {
          mClickBool=false;
          mouse.middleClickUp=true;
        }
        if(mClick--<1) mClick=0;
        if(rClick) {
          rClick = false;
          mouse.rightClickUp=true;
        }
      }
    }
    //OSA_TimeDelay(1000);

  }
  return;
}

static void prvDebugTask(void *pvParameters) {
  while(1) {
    PRINTF("L: %d %d M: %d %d R: %d %d, X %d Y %d S %d\r\n",mouse.leftClickDown,mouse.leftClickUp,mouse.middleClickDown,mouse.middleClickUp,mouse.rightClickDown,mouse.rightClickUp,mouse.x,mouse.y,mouse.scroll);
    vTaskDelay(50);
  }
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

  xTaskCreate(prvHardwareMonitorTask, (const char*) "hwMon",configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL);
  xTaskCreate(prvDebugTask, (const char*) "debug",configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
  vTaskStartScheduler();


  while(1) { // If we fell through and had a horrible error
    LED1_ON;LED2_ON;
    OSA_TimeDelay(200);
    LED1_OFF;LED2_OFF;
    OSA_TimeDelay(200);
  }

  return 1;
}

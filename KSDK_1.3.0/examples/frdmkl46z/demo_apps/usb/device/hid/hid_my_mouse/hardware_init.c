// Original file from Freescale Semiconductor
// Edited with James Adams for I2C communication

#include "board.h"
#include "MKL46Z4.h"
#include "pin_mux.h"
#include "fsl_clock_manager.h"
#include "fsl_debug_console.h"

#include "fsl_i2c_hal.h"
#include "fsl_i2c_master_driver.h"

#define _BV(bit) (1U << bit)

#define ACCEL_ADDR 0x1d
#define ACCEL_READ_CMD 0x3B
#define ACCEL_WRITE_CMD 0x3A

#define kMMA8451_STATUS 0x00
#define kMMA8451_OUT_X_MSB 0x01
#define kMMA8451_OUT_X_LSB 0x02
#define kMMA8451_OUT_Y_MSB 0x03
#define kMMA8451_OUT_Y_LSB 0x04
#define kMMA8451_OUT_Z_MSB 0x05
#define kMMA8451_OUT_Z_LSB 0x06
#define kMMA8451_F_SETUP 0x09
#define kMMA8451_TRIG_CFG 0x0a
#define kMMA8451_SYSMOD 0x0b
#define kMMA8451_INT_SOURCE 0x0c
#define kMMA8451_WHO_AM_I 0x0d
#define kMMA8451_WHO_AM_I_Device_ID 0x1a
#define kMMA8451_XYZ_DATA_CFG 0x0e
#define kMMA8451_CTRL_REG1 0x2a
#define kMMA8451_CTRL_REG2 0x2b
#define kMMA8451_CTRL_REG4 0x2d
#define kMMA8451_CTRL_REG3 0x2c
#define kMMA8451_CTRL_REG5 0x2e

#define I2CDEBUG

void PRINTINT(uint16_t b) {
  char buf[6];
  snprintf(buf,sizeof(buf),"%d",b);
  PRINTF(buf);
  return;
}

void PRINTHEX(uint16_t b) {
  char buf[6];
  snprintf(buf,sizeof(buf),"%x",b);
  PRINTF(buf);
  return;
}

void raw_i2c_write(uint8_t addr, uint8_t reg, uint8_t byte) {
  #ifdef I2CDEBUG
  PRINTF("Entered I2C write function, sending start\r\n");
  #endif
  I2C0_C1 |= I2C_C1_TX(1); // Make sure we are in master transmit

  if(I2C0_C1 & I2C_C1_MST_MASK) {
    // We must be already in master mode, send a repeated start
    I2C0_C1 |= I2C_C1_RSTA(1);
  } else {
    // Enter master mode
    I2C0_C1 |= I2C_C1_MST(1);
  }

  #ifdef I2CDEBUG
  PRINTF("Sending address ");
  PRINTHEX(addr);
  PRINTF("\r\n");
  #endif

  // Send the address
  I2C0_D = addr;

  // Wait for transfer to complete
  while(!(I2C0_S & I2C_S_TCF_MASK));

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(I2C0_S & I2C_S_RXAK_MASK);
  PRINTF("\r\n");

  PRINTF("Sending register address ");
  PRINTHEX(reg);
  PRINTF("\r\n");
  #endif

  // Send the register address
  I2C0_D = reg;

  // Wait for transfer to complete
  while(!(I2C0_S & I2C_S_TCF_MASK));

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(I2C0_S & I2C_S_RXAK_MASK);
  PRINTF("\r\n");

  PRINTF("Sending byte ");
  PRINTHEX(byte);
  PRINTF("\r\n");
  #endif

  // Send the register address
  I2C0_D = byte;

  // Wait for transfer to complete
  while(!(I2C0_S & I2C_S_TCF_MASK));

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(I2C0_S & I2C_S_RXAK_MASK);
  PRINTF("\r\n");

  PRINTF("Send stop bit\r\n");
  #endif
  I2C0_C1 &= ~(I2C_C1_TX(1) | I2C_C1_MST(1)); // Set TX and MST to 0 (go into receive + send stop)

  // Wait for stop signal to send
  while(I2C0_S & I2C_S_BUSY_MASK);

  #ifdef I2CDEBUG
  PRINTF("Finished sending I2C byte\r\n\r\n");
  #endif
  return;


  /*
  I2C0_C1 |= I2C_C1_TX(1); // Make sure we are in master transmit mode
  while(!(I2C0_S & 0x80)); // Wait for a previous transfer to be complete
  I2C0_D = byte; // This should auto-clock out the data
  /* Can also read in 8 bits that were clocked technically, not sure if works
  while(I2C0_S & I2C_S_RXAK);
  return I2C0_D;
  return;*/
}

uint8_t raw_i2c_read(uint8_t write_addr, uint8_t read_addr, uint8_t reg) {
  uint32_t base = I2C0_BASE;
  int instance = 0;
  int timeout_ms = 100;

  I2C_HAL_SetDirMode(I2C0_BASE, 1U);
  I2C_HAL_ClearInt(base);
  I2C_HAL_SendStart(base);

  I2C_HAL_WriteByte(base, write_addr);
  PRINTF("Wrote write_addr\r\n");
  /* Wait for the transfer to finish.*/
  while(!(I2C0_S & I2C_S_TCF_MASK));
  I2C_HAL_WriteByte(base,reg);
  PRINTF("Wrote reg\r\n");
  while(!(I2C0_S & I2C_S_TCF_MASK));

  I2C_HAL_WriteByte(base,read_addr);
  PRINTF("Wrote read_addr\r\n");
  while(!(I2C0_S & I2C_S_TCF_MASK));

  I2C_HAL_SetDirMode(base, 0);
  PRINTF("Went to receive mode\r\n");

  I2C_HAL_SendNak(base);
  PRINTF("Sent Nak for some reason\r\n");

  /* Dummy read to trigger receive of next byte in interrupt. */
  I2C_HAL_ReadByte(base);

  while(!(I2C0_S & I2C_S_TCF_MASK));
     /* Disable interrupt. */
     I2C_HAL_SetIntCmd(base, false);

     if (I2C_HAL_GetStatusFlag(base, kI2CBusBusy))
     {
       /* Generate stop signal. */
       I2C_HAL_SendStop(base);
       PRINTF("Sent stop, I don't know where our data is\r\n");
     }

  return I2C0_D;















  #ifdef I2CDEBUG
  PRINTF("Entered I2C read function, sending start\r\n");
  #endif
  I2C0_C1 |= I2C_C1_TX(1); // Make sure we are in master transmit

  if(I2C0_C1 & I2C_C1_MST_MASK) {
    // We must be already in master mode, send a repeated start
    I2C0_C1 |= I2C_C1_RSTA(1);
  } else {
    // Enter master mode
    I2C0_C1 |= I2C_C1_MST(1);
  }

  #ifdef I2CDEBUG
  PRINTF("Sending write address ");
  PRINTHEX(write_addr);
  PRINTF("\r\n");
  #endif

  // Send the address
  I2C0_D = write_addr;
  //for(int i=0;i<0x400;i++) __asm("nop");
  OSA_TimeDelay(25);

  // Wait for transfer to complete
  while(!(I2C0_S & I2C_S_TCF_MASK));

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(I2C0_S & I2C_S_RXAK_MASK);
  PRINTF("\r\n");

  PRINTF("Sending register address ");
  PRINTHEX(reg);
  PRINTF("\r\n");
  #endif

  // Send the register address
  I2C0_D = reg;
  //for(int i=0;i<0x400;i++) __asm("nop");
  OSA_TimeDelay(25);

  // Wait for transfer to complete
  while(!(I2C0_S & I2C_S_TCF_MASK));

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(I2C0_S & I2C_S_RXAK_MASK);
  PRINTF("\r\n");
  #endif

  // send a repeated start
  I2C0_C1 |= I2C_C1_RSTA(1);
  #ifdef I2CDEBUG
  PRINTF("Sent repeated start\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Sending read address ");
  PRINTHEX(read_addr);
  PRINTF("\r\n");
  #endif

  // Send the address
  I2C0_D = read_addr;
  //for(int i=0;i<0x400;i++) __asm("nop");
  OSA_TimeDelay(25);

  // Wait for transfer to complete
  while(!(I2C0_S & I2C_S_TCF_MASK));

  #ifdef I2CDEBUG
  PRINTF("Entering read mode\r\n");
  #endif

  I2C0_C1 &= ~I2C_C1_TX(1);

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(I2C0_S & I2C_S_RXAK_MASK);
  PRINTF("\r\n");
  #endif

  // Wait for transfer to complete
  while(!(I2C0_S & I2C_S_TCF_MASK));
  //for(int i=0;i<0x400;i++) __asm("nop");
  OSA_TimeDelay(25);

  uint8_t data;
  data = I2C0_D;

  #ifdef I2CDEBUG
  PRINTF("RECEIVED: ");
  PRINTHEX(data);
  PRINTF("\r\n");

  PRINTF("Sending NAK and STOP\r\n");
  #endif

  I2C0_C1 |= I2C_C1_TXAK(1); // send NACK

  I2C0_C1 &= ~(I2C_C1_TX(1) | I2C_C1_MST(1)); // Set TX and MST to 0 (go into receive + send stop)

  // Wait for stop signal to send
  while(I2C0_S & I2C_S_BUSY_MASK);

  #ifdef I2CDEBUG
  PRINTF("Finished receiving I2C byte\r\n\r\n");
  #endif
  return data;

}

void mma8451_write(uint8_t addr, uint8_t byte) {
  /*
  I2C_SEND_START();
  raw_i2c_write(ACCEL_WRITE_CMD);
  raw_i2c_write(addr);
  raw_i2c_write(byte);
  I2C_SEND_STOP();*/
}

uint8_t mma8451_read(uint8_t addr) {
  /*
  PRINTF("Sending start\r\n");
  I2C_SEND_START();
  PRINTF("YEP\r\n");
  raw_i2c_write(ACCEL_WRITE_CMD);
  raw_i2c_write(addr);
  while(!(I2C0_S & 0x80)); // Wait for a previous transfer to be complete
  I2C0_C1 |= I2C_C1_RSTA(1); // send repeated start
  raw_i2c_write(ACCEL_READ_CMD);
  return raw_i2c_read();*/
}

void mma8451_init() {


  PRINTF("Initialising registers on mma8451\r\n");
  uint8_t val = 0;
  // Put into standby mode whilst we configure it
  val = mma8451_read(kMMA8451_CTRL_REG1);
  val &= (uint8_t)(~(0x01));
  mma8451_write(kMMA8451_CTRL_REG1, val);

  // Set the range, -4g to 4g
  val = mma8451_read(kMMA8451_XYZ_DATA_CFG);
  val &= (uint8_t)(~0x03);
  val |= 0x01;
  mma8451_write(kMMA8451_XYZ_DATA_CFG, val);

  // Set the F_MODE, disable FIFO
  val = mma8451_read(kMMA8451_F_SETUP);
  val &= 0x3F;
  mma8451_write(kMMA8451_F_SETUP, val);

  // Put the mma8451 into active mode
  val = mma8451_read(kMMA8451_CTRL_REG1);
  val |= 0x01;
  val &= (uint8_t)(~0x02);               /* set F_READ to 0 */
  mma8451_write(kMMA8451_CTRL_REG1, val);

  PRINTF("Finished registers on mma8451\r\n");

  return;
}

void raw_i2c_init(void) {

  PRINTF("Initialising I2C..");

  // Enable port clocks
  //SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
  CLOCK_SYS_EnablePortClock(PORTA_IDX);
  CLOCK_SYS_EnablePortClock(PORTD_IDX);
  CLOCK_SYS_EnablePortClock(PORTE_IDX);


  PRINTF("Enabled port clocks..");

  // Enable I2C pins
  //PORTE->PCR[24] = _BV(10)|_BV(8); // Set pin 24 of PORTE as I2C (alternative mode 5), passive filter, pullup enable and pull select
  //PORTE->PCR[25] = _BV(10)|_BV(8); // Set pin 25 of PORTE as I2C, and the same ^
  PORT_HAL_SetMuxMode(PORTE,24u,kPortMuxAlt5);
  /* PORTE_PCR25 */
  PORT_HAL_SetMuxMode(PORTE,25u,kPortMuxAlt5);

  CLOCK_SYS_EnableI2cClock(0);

  /* Initialize peripheral to known state.*/
  I2C_HAL_Init(I2C0_BASE);
  I2C0_F = I2C_F_MULT(4U) | I2C_F_ICR(20U); // This should run at somewhere below 1MHz (48/(4*20))
  I2C_HAL_Enable(I2C0_BASE);
  PRINTF("Did their shit\r\n");
  return;















  //PORTB->PCR[2] = _BV(9) | _BV(4) | _BV(1) | _BV(0); // Set pin 2 of PORTB as I2C (alternative mode 2), passive filter, pullup enable and pull select
  //PORTB->PCR[3] = _BV(9) | _BV(4) | _BV(1) | _BV(0); // Set pin 3 of PORTB as I2C, and the same ^

  PRINTF("Enabled I2C pins..");

  // Enable clock to I2C
  SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK; //clock to I2C0
  PRINTF("Clock routed to I2C..");

  // Initialise I2C registers to 0
  I2C0_A1 = 0;
  I2C0_F = 0;
  I2C0_C1 = 0;
  I2C0_S = 0;
  I2C0_C2 = 0;
  I2C0_FLT = 0;
  I2C0_RA = 0;

  PRINTF("I2C Registers set to 0\r\n");

  // Now enable I2C
  I2C0_C1 |= I2C_C1_IICEN(1);
  I2C0_F = I2C_F_MULT(4U) | I2C_F_ICR(20U); // This should run at somewhere below 1MHz (48/(4*20))


  PRINTF("Finished initialising I2C\r\n");

  return;

  /*
  PRINTF("Initialising clocks for I2C\r\n");

  // Clock setup from https://community.nxp.com/thread/384832
  SIM_CLKDIV1 = SIM_CLKDIV1 | (1u<<17) | (1u<<16); //bus clock is 24/3 = 8MHz
  SIM_SCGC5 = SIM_SCGC5 | SIM_SCGC5_PORTE_MASK; //clock to PTE24 and PTE25 for I2C0
  SIM_SCGC4 = SIM_SCGC4 | SIM_SCGC4_I2C0_MASK; //clock to I2C0
  PORTE_PCR24 = PORTE_PCR24 | (1u<<10) & ~(1u<<9) | (1u<<8); //alternative 5 - 101 for bits 10, 9 8 respectively
  PORTE_PCR25 = PORTE_PCR25 | (1u<<10) & ~(1u<<9) | (1u<<8); //alternative 5 - 101 for bits 10, 9 8 respectively

  PRINTF("Initialising i2c registers\r\n");
  I2C0_F = I2C_F_MULT(2) | I2C_F_ICR(0); // Set the I2C bus to some value of clock speed
  PRINTF("Set I2C clock pre-scaling\r\n");
  I2C0_C1 = I2C_C1_IICEN(1); // Enable the I2C
  PRINTF("Finished setting up i2c interface\r\n");
  I2C0_C2 = 0;
  return;*/
}

void hardware_init(void)
{

    uint8_t i;

    /* enable clock for PORTs */
    for (i = 0; i < PORT_INSTANCE_COUNT; i++)
    {
        CLOCK_SYS_EnablePortClock(i);
    }

    /* Init board clock */

    BOARD_ClockInit();
    dbg_uart_init();

    OSA_Init();

    PRINTF("\r\n\r\nInitiated debug uart and board clocks\r\n");
    raw_i2c_init();

    //raw_i2c_write(ACCEL_WRITE_CMD,kMMA8451_CTRL_REG1,0);
    //mma8451_init();



    PRINTF("Pulling register: ");
    uint16_t eh = 0;
    eh = raw_i2c_read(ACCEL_READ_CMD,ACCEL_WRITE_CMD,kMMA8451_WHO_AM_I);
    PRINTINT(eh);
    PRINTF(".\r\n");

}

/*!
 ** @}
 */
/*
 ** ###################################################################
 **
 **     This file was created by Processor Expert 10.4 [05.10]
 **     for the Freescale Kinetis series of microcontrollers.
 **
 ** ###################################################################
 */

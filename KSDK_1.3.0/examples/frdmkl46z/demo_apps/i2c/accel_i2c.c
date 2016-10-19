#include "board.h"
#include "MKL46Z4.h"

//#define I2CDEBUG

#ifdef I2CDEBUG
#include "fsl_debug_console.h"
#endif

#define _BV(bit) (1U << bit)

#define ACCEL_ADDR 0x1d
#define ACCEL_READ_CMD 0x3B
#define ACCEL_WRITE_CMD 0x3A

#define ACCEL_CTRL_1 0x2a
#define ACCEL_OUT_X_MSB 0x01

void udelay(int num) {
  for(int i=0;i<num;i++) asm("nop");
  return;
}

#ifdef I2CDEBUG
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
#endif


uint8_t read_i2c(uint8_t write_addr,uint8_t read_addr, uint8_t reg) {
  uint8_t lastStatus = 0;
  I2C0_C1 |= I2C_C1_TX(1); // Make sure we are in master transmit
  I2C0_C1 |= I2C_C1_MST(1); // enter master mode

  #ifdef I2CDEBUG
  PRINTF("Sending write address ");
  PRINTHEX(write_addr);
  PRINTF("\r\n");
  #endif

  // Send the address
  I2C0_D = write_addr;
  while(!((lastStatus = I2C0_S) & 0b10000000)); // wait for transfer complete + get status register
  // Note that some flags in this register are volatile (get cleared after reading)
  udelay(300);

  #ifdef I2CDEBUG
  PRINTF("Status: ");PRINTHEX(lastStatus);PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(!(lastStatus & I2C_S_RXAK_MASK));
  PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Sending reg address ");
  PRINTHEX(reg);
  PRINTF("\r\n");
  #endif

  I2C0_D = reg;
  udelay(300);

  while(!((lastStatus = I2C0_S) & 0b10000000)); // wait for transfer complete + get status register
  // Note that some flags in this register are volatile (get cleared after reading)
  #ifdef I2CDEBUG
  PRINTF("Status: ");PRINTHEX(lastStatus);PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(!(lastStatus & I2C_S_RXAK_MASK));
  PRINTF("\r\n");
  #endif


  // Send repeated start

  I2C0_C1 |= _BV(2);



  #ifdef I2CDEBUG
  PRINTF("Sending read address ");
  PRINTHEX(read_addr);
  PRINTF("\r\n");
  #endif

  // Send the address
  I2C0_D = read_addr;

  while(!((lastStatus = I2C0_S) & 0b10000000)); // wait for transfer complete + get status register
  // Note that some flags in this register are volatile (get cleared after reading)

  udelay(300);

  #ifdef I2CDEBUG
  PRINTF("Status: ");PRINTHEX(lastStatus);PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(!(lastStatus & I2C_S_RXAK_MASK));
  PRINTF("\r\n");
  #endif

  I2C0_C1 &= ~I2C_C1_TX(1); // Receive mode

  uint8_t data=0;
  data = I2C0_D; // cause a read to happen

  while(!((lastStatus = I2C0_S) & 0b10000000)); // wait for transfer complete + get status register
  // Note that some flags in this register are volatile (get cleared after reading)

  #ifdef I2CDEBUG
  PRINTF("Status: ");PRINTHEX(lastStatus);PRINTF("\r\n");
  #endif

  udelay(300);

  I2C0_C1 |= _BV(3); // TXAK - we want to send a NAK
  data = I2C0_D; // now read the actual data

  udelay(300);

  I2C0_C1 |= I2C_C1_TX(1);
  I2C0_C1 &= ~I2C_C1_MST(1); // Send a stop bit

  while(!(I2C0_S & _BV(5))); // Wait for stop to be sent

  #ifdef I2CDEBUG
  PRINTF("Data: ");PRINTHEX(data);PRINTF("\r\n");
  #endif
  // reset TXAK to 0
  I2C0_C1 &= ~_BV(3);

  return data;
}

void write_i2c(uint8_t write_addr, uint8_t reg, uint8_t data) {
  uint8_t lastStatus = 0;
  I2C0_C1 |= I2C_C1_TX(1); // Make sure we are in master transmit
  I2C0_C1 |= I2C_C1_MST(1); // enter master mode

  #ifdef I2CDEBUG
  PRINTF("Sending write address ");
  PRINTHEX(write_addr);
  PRINTF("\r\n");
  #endif

  // Send the address
  I2C0_D = write_addr;
  //for(int i=0;i<0x400;i++) __asm("nop");
  // Wait for transfer to complete
  while(!((lastStatus = I2C0_S) & 0b10000000)); // wait for transfer complete + get status register
  // Note that some flags in this register are volatile (get cleared after reading)
  udelay(300);

  #ifdef I2CDEBUG
  PRINTF("Status: ");PRINTHEX(lastStatus);PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(!(lastStatus & I2C_S_RXAK_MASK));
  PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Sending reg address ");
  PRINTHEX(reg);
  PRINTF("\r\n");
  #endif

  I2C0_D = reg;

  while(!((lastStatus = I2C0_S) & 0b10000000)); // wait for transfer complete + get status register
  // Note that some flags in this register are volatile (get cleared after reading)
  udelay(300);

  #ifdef I2CDEBUG
  PRINTF("Status: ");PRINTHEX(lastStatus);PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(!(lastStatus & I2C_S_RXAK_MASK));
  PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Sending data ");
  PRINTHEX(data);
  PRINTF("\r\n");
  #endif

  I2C0_D = data;

  while(!((lastStatus = I2C0_S) & 0b10000000)); // wait for transfer complete + get status register
  // Note that some flags in this register are volatile (get cleared after reading)
  udelay(300);

  #ifdef I2CDEBUG
  PRINTF("Status: ");PRINTHEX(lastStatus);PRINTF("\r\n");
  #endif

  #ifdef I2CDEBUG
  PRINTF("Acknowledged? ");
  PRINTINT(!(lastStatus & I2C_S_RXAK_MASK));
  PRINTF("\r\n");
  #endif

  I2C0_C1 &= ~I2C_C1_MST(1); // Send stop bit
  return;
}

void init_i2c() {
  SIM->SCGC5 |= _BV(13); // enable port E
  SIM->SCGC4 |= _BV(6); // Enable I2C Clock

  I2C0_A1 = 0;
  I2C0_F = 0;
  I2C0_C1 = 0;
  I2C0_S = 0;
  I2C0_C2 = 0;
  I2C0_FLT = 0;
  I2C0_RA = 0;

  PORTE_PCR24 |= _BV(8) | _BV(10); // Set pin 24 to mux mode alt 5 (I2C0)
  PORTE_PCR25 |= _BV(8) | _BV(10); // Set pin 24 to mux mode alt 5 (I2C0)

  I2C0_F = 0b00001000; //  8 for divider
  I2C0_C1 = I2C_C1_IICEN(1) | I2C_C1_IICIE(1); // enable I2C
  I2C0_D = 0; // Initialise variables
  //PRINTF("Initialised I2C\r\n");

  return;
}

void hardware_init() {
  /* enable clock for PORTs */
  SIM->SCGC5 |= _BV(13)|_BV(12)|_BV(11)|_BV(10)|_BV(9);

  BOARD_ClockInit();
  dbg_uart_init();

  //PRINTF("Initialised debug\r\n\r\n");

  init_i2c();

  return;
}

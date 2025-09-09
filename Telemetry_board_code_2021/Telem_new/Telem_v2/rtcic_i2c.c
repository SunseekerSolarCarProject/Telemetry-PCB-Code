/*
 * i2c.c
 *
 *  Created on: Jun 2, 2021
 *      Author: Bazuin
 */

#include "Sunseeker2021.h"

// Taken from CAPE display initialization of I2C interface
void init_i2c(void)
{

// Setup USI Master TX
  P9DIR |=  RTC_SDA;					// Set to output
  P9DIR |=  RTC_SCL;					// Set to output

  P9SEL |= RTC_SDA;                     // Assign I2C pins to USCI_B1
  P9SEL |= RTC_SCL;                     // Assign I2C pins to USCI_B1

  UCB2CTL1 |= UCSWRST;                      // Enable SW reset
  UCB2CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
  UCB2CTL1 = UCSSEL_3 + UCSWRST;            // Use SMCLK, keep SW reset
  UCB2BR0 = 80;                             // fSCL = SMCLK/80 = ~125 kHz
  UCB2BR1 = 0;
  UCB2I2CSA = SLV_Addr;                    // Slave Address
  UCB2CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  UCB2IFG &= ~UCTXIFG;                       // Clear USCI_B1 TX int flag
//  UCB2IE |= UCTXIE;                          // Enable TX interrupt

}

void init_MCP7940M(void)
{
	unsigned int ii;
	unsigned char *PTxData;                     // Pointer to TX data
	unsigned char TXByteCtr;
	const unsigned char TxData[] =              // Table of data to transmit
	{
	  0x00, // Register 0 Address
	  0x80, // turn on internal osc, sec = 0
	  0x00, // min = 0
	  0x92, // 24-hour format, hours = 12
	  0x01, // weekday = Monday
	  0x01, // January 1
	  0x01, //
	  0x21,   // xx21 (BCD)
	  0x40  // output 1 Hz, no alarm, use crystal osc
	};

    PTxData = (unsigned char *)TxData;      // TX array start address
    TXByteCtr = sizeof TxData;              // Load TX byte counter

    while (UCB2CTL1 & UCTXSTP);             // Ensure stop condition got sent
	while (UCB2STAT & (UCBBUSY | UCSCLLOW));
    UCB2CTL1 |= UCTR;             // I2C TX, start condition
    UCB2CTL1 |= UCTXSTT;             // I2C TX, start condition
    while ((UCB2IFG & UCTXIFG)==0);

    for (ii =TXByteCtr;ii>0;ii--){
        UCB2TXBUF = *PTxData++;                 // Load TX buffer
        while ((UCB2IFG & UCTXIFG)==0);
    }

    UCB2CTL1 |= UCTXSTP;                    // I2C stop condition
    while (UCB2CTL1 & UCTXSTP);
}

void set_all_MCP7940M(char h, char m, char s, char mo, char d, char y)
{
    while (UCB2CTL1 & UCTXSTP);             // Ensure stop condition got sent
	while (UCB2STAT & (UCBBUSY | UCSCLLOW));
    UCB2CTL1 |= UCTR;             // I2C TX, start condition
    UCB2CTL1 |= UCTXSTT;             // I2C TX, start condition
    while ((UCB2IFG & UCTXIFG)==0);

    UCB2TXBUF = 0x00;            // RTCSEC Address
    while ((UCB2IFG & UCTXIFG)==0);
    // Note: all values are BCD
    UCB2TXBUF = s + 0x80;          // RTCSEC with internal osc enabled
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = m;                 // RTCMIN
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = h;                 // RTCHOUR
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = 0;                 // RTCWKDAY
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = d;                 // RTCDATE
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = mo;                // RTCMTH
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = y;                 // RTCYEAR
    while ((UCB2IFG & UCTXIFG)==0);

    UCB2CTL1 |= UCTXSTP;                    // I2C stop condition
    while (UCB2CTL1 & UCTXSTP);
}

void set_MCP7940M(char h, char m, char s)
{
    while (UCB2CTL1 & UCTXSTP);             // Ensure stop condition got sent
	while (UCB2STAT & (UCBBUSY | UCSCLLOW));
    UCB2CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
    while ((UCB2IFG & UCTXIFG)==0);

    UCB2TXBUF = 0x00;            // RTCSEC Address
    while ((UCB2IFG & UCTXIFG)==0);
    // Note: all values are BCD
    UCB2TXBUF = s + 0x80;          // RTCSEC with internal osc enabled
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = m;                 // RTCMIN
    while ((UCB2IFG & UCTXIFG)==0);
    UCB2TXBUF = h;                 // RTCHOUR
    while ((UCB2IFG & UCTXIFG)==0);

    UCB2CTL1 |= UCTXSTP;                    // I2C stop condition
    while (UCB2CTL1 & UCTXSTP);
}

void get_MCP7940M(unsigned char *hr, unsigned char *min, unsigned char *sec)
{
	unsigned char sec_raw, min_raw, hrs_raw;
    while (UCB2CTL1 & UCTXSTP);             // Ensure stop condition got sent
	while (UCB2STAT & (UCBBUSY | UCSCLLOW));
    UCB2CTL1 |= UCTR;             // I2C TX, start condition
    UCB2CTL1 |= UCTXSTT;             // I2C TX, start condition
    //while ((UCB2CTL1 & UCTXSTT)==0);
    //while ((UCB2IFG & UCSTTIFG)==0);

    UCB2TXBUF = 0x00;            // RTCSEC Address
    while ((UCB2IFG & UCTXIFG)==0);
    //UCB2CTL1 |= UCTXSTP;                    // I2C stop condition
    //while (UCB2CTL1 & UCTXSTP);


    UCB2CTL1 &= ~UCTR;             // I2C RX
    UCB2CTL1 |= UCTXSTT;           // start condition
    while (UCB2CTL1 & UCTXSTT);

    while ((UCB2IFG & UCRXIFG)==0);
    sec_raw = UCB2RXBUF;
    *sec = sec_raw & 0x7F;

    while ((UCB2IFG & UCRXIFG)==0);
    min_raw = UCB2RXBUF;
    *min = min_raw;

    UCB2CTL1 |= UCTXSTP;                    // I2C stop condition
    while ((UCB2IFG & UCRXIFG)==0);
    hrs_raw = UCB2RXBUF;
    *hr = hrs_raw & 0x3F;

    while (UCB2CTL1 & UCTXSTP);

}

//
/* prototype write
 * TXtype = 0;
 * PTxData = (unsigned char *)Command1;  // TX array start address
                                            // Place breakpoint here to see each
                                            // transmit operation.
  TXByteCtr = sizeof LCD_Command1;          // Load TX byte counter

 * i2c_start() ; // start I2C communication: SDA goes down while SCL remains high
 * -  UCB1CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
 * i2c_wr(ADDR_RTCC_WRITE); // send the RTCC's address for write = 0xde
 * i2c_wr(rtcc_reg) ; // send the register's address
 * i2c_wr (time_var) ; // send the data byte
 * i2c_stop() ; // stop I2C communication: SDA goes high while SCL remains high
 * - while (UCB1CTL1 & UCTXSTP);             // Ensure stop condition got sent
 *
 * __delay_cycles(4000);             // MCLK cycles delay cycles
 */

/* prototype read
 * i2c_start() ; // start I2C communication: SDA goes down while SCL remains high
 * i2c_wr(ADDR_RTCC_WRITE); // send the RTCC's address for write = 0xde
 * i2c_wr(rtcc_reg) ; // send the register's address
 * i2c_restart() ; // switch to reads
 * i2c_wr(ADDR_RTCC_READ) ; // send the RTCC's address for read = 0xdf
 * i2c_rd() ; // read the byte from the RTCC (register's content)
 * i2c_nack ; // NoACK from MCU to the RTCC (no more bytes to read)
 * i2c_stop() ; // stop I2C communication: SDA goes high while SCL remains high
 */

//
//   char time_msg[17];// = "TL_TIM,HH:MM:SS\r\n";
//
int insert_time_2(char *time_string)
{
  char h1, h0, m1, m0, s1, s0;
  extern char bhrs, bmin, bsec;

  h1 = ((bhrs>>4) & 0x0F)+'0';
  h0 = (bhrs & 0x0F)+'0';
  time_string[7] = h1;
  time_string[8] = h0;

  m1 = ((bmin>>4) & 0x0F)+'0';
  m0 = (bmin & 0x0F)+'0';
  time_string[10] = m1;
  time_string[11] = m0;

  s1 = ((bsec>>4) & 0x0F)+'0';
  s0 = (bsec & 0x0F)+'0';
  time_string[13] = s1;
  time_string[14] = s0;

  return 1;
}

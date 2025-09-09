/*
 * - Implements the following UCB0 interface functions
 *	- init
 *	- transmit
 *	- exchange
 *
 */

// Include files
#include<msp430FR2476.h>

/*
 * Initialise UCB0 port
 */
 
// Public Function prototypes
void canspi_init(void)
{
    UCB0CTLW1 |= UCSWRST;					//software reset
    UCB0CTLW0 |= UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC; //data-capt then change; MSB first; Master; 3-pin SPI; sync
	UCB0CTLW1 |= UCSSEL_2;				//set SMCLK
	UCB0BR0 = 0x01;					//set clk prescaler to 1
	UCB0BR1 = 0x00;
	UCB0STATW = 0x00;					//not in loopback mode
	UCB0CTLW1 &= ~UCSWRST;				//SPI enable turn off software reset
	//UCB0IE |= UCTXIE | UCRXIE;		// Interrupt Enable
}

/*
* Transmits data on UCB0 connection
*	- Busy waits until entire shift is complete
*/
void canspi_transmit(unsigned char data)
{
  unsigned char forceread;
  UCB0TXBUF = data;
  while((UCB0IFG & UCTXIFG) == 0x00);	// Wait for Rx completion (implies Tx is also complete)
  forceread = UCB0RXBUF;
}

/*
* Exchanges data on UCB0 connection
*	- Busy waits until entire shift is complete
*	- This function is safe to use to control hardware lines that rely on shifting being finalised
*/
unsigned char canspi_exchange(unsigned char data)
{
  UCB0TXBUF = data;
  while((UCB0IV & UCB0IFG) == 0x00);	// Wait for Rx completion (implies Tx is also complete)
  return(UCB0RXBUF);
}
 

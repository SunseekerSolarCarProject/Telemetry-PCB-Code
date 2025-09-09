/*
 * - Implements the following UCB1 interface functions
 *	- init
 *	- transmit
 *	- exchange
 *
 */

// Include files
#include<msp430fr2476.h>

/*
 * Initialise UCB1 port
 */
 
// Public Function prototypes
void canspi_or_init(void)
{
	UCB1CTL1 |= UCSWRST;					//software reset
	UCB1CTLW0 |= UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC | UCSSEL_2; //data-capt then change; MSB first; Master; 3-pin SPI; sync
//	                            			//set SMCLK
	UCB1BR0 = 0x01;					//set clk prescaler to 1
	UCB1BR1 = 0x00;
	UCB1STATW = 0x00;					//not in loopback mode
	UCB1CTL1 &= ~UCSWRST;				//SPI enable turn off software reset
	// UCB1IE |= UCTXIE | UCRXIE;		// Interrupt Enable
}

/*
* Transmits data on UCB1 connection
*	- Busy waits until entire shift is complete
*/
void canspi_or_transmit(unsigned char data)
{
  unsigned char forceread;
  UCB1TXBUF = data;
  while((UCB1IFG & UCRXIFG) == 0x00);	// Wait for Rx completion (implies Tx is also complete)
  forceread = UCB1RXBUF;
}

/*
* Exchanges data on UCB1 connection
*	- Busy waits until entire shift is complete
*	- This function is safe to use to control hardware lines that rely on shifting being finalised
*/
unsigned char canspi_or_exchange(unsigned char data)
{
  UCB1TXBUF = data;
  while((UCB1IFG & UCRXIFG) == 0x00);	// Wait for Rx completion (implies Tx is also complete)
  return(UCB1RXBUF);
}
 

/*
 * Initialise UCB0 port
 */

// Public Function prototypes
void canspi_init(void)
{
    UCB0CTL1 |= UCSWRST;                //software reset
    UCB0CTLW0 |= UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC | UCSSEL_2; //data-capt then change; MSB first; Master; 3-pin SPI; sync
//                                      //set SMCLK
    UCB0BR0 = 0x01;                     //set clk prescaler to 1
    UCB0BR1 = 0x00;
    UCB0STATW = 0x00;                   //not in loopback mode
    UCB0CTL1 &= ~UCSWRST;               //SPI enable turn off software reset
    // UCB0IE |= UCTXIE | UCRXIE;       // Interrupt Enable
}

/*
* Transmits data on UCB0 connection
*   - Busy waits until entire shift is complete
*/
void canspi_transmit(unsigned char data)
{
  unsigned char forceread;
  UCB0TXBUF = data;
  while((UCB0IFG & UCRXIFG) == 0x00);   // Wait for Rx completion (implies Tx is also complete)
  forceread = UCB0RXBUF;
}

/*
* Exchanges data on UCB0 connection
*   - Busy waits until entire shift is complete
*   - This function is safe to use to control hardware lines that rely on shifting being finalised
*/
unsigned char canspi_exchange(unsigned char data)
{
  UCB0TXBUF = data;
  while((UCB0IFG & UCRXIFG) == 0x00);   // Wait for Rx completion (implies Tx is also complete)
  return(UCB0RXBUF);
}






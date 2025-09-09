/*
 *  Originally Written for BPS2012 PCB Development
 * 
 *  Modified for BPS_V2 2015 by Scott Haver
 *
 *  Clock and microcontroller Initialization RS232
 */

// Include files
#include "Sunseeker2024.h"
#include "UART.h"

/*********************************************************************************/
// BPS to PC External RS-232 (voltage isolated)
/*********************************************************************************/
void CPUART_init(void)
{
    UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK = 3,997,696
//    UCA0BRW = 6;                            // 3997696 Hz to 38400(see User's Guide)
//    UCA0MCTLW |= 0x1100 + UCBRF_8 + UCOS16;            // Modulation UCBRSx=11, UCBRFx=1
    UCA0BRW = 26;                            // 3997696 Hz to 9600 = 416.42, /16 = 26.02 (see User's Guide)
    UCA0MCTLW |= 0x5300 + UCBRF_0 + UCOS16;            // Modulation UCBRSx=53, UCBRFx=0
	
	UCA0IFG &= ~UCTXIFG;			//Clear Xmit and Rec interrupt flags
	UCA0IFG &= ~UCRXIFG;
	UCA0CTL1 &= ~UCSWRST;			// initalize state machine
//  UCA0IE |= UCRXIE|UCTXIE;		//enable TX & RX interrupt
}

void CPUART_putchar(char data)
{
    while((UCA0IFG & UCTXIFG) == 0);
    UCA0TXBUF = data;
}

unsigned char CPUART_getchar(void)
{
	char i = 100;

    while((UCA0IFG & UCRXIFG) == 0 && i != 0)
    {
    	i--;
    }
    return(UCA0RXBUF);
}

int CPUART_puts(char *str)
// polling string transmission
{
	int i;
    char ch;
    i = 0;

    while((ch=*str)!= '\0')
    {
    	CPUART_putchar(ch);
    	str++;
    	i++;
    }
    CPUART_putchar(0x0A);
    CPUART_putchar(0x0D);
    
    return(i);
}

int CPUART_gets(char *ptr)
{
    int i;
    i = 0;
    while (1) {
          *ptr = CPUART_getchar();
          if (*ptr == 0x0D){
             *ptr = 0;
             return(i);
          }
          else 
          {
          	ptr++;
          	i++;
          }
     }
}

void CPUART_puts_int(void)
{
    extern char *CPUART_TX_ptr;
    extern char put_status_CPUART;
    char ch;
    
    ch = *CPUART_TX_ptr++;

	if (ch == '\0')
	{
		UCA0IE &= ~UCTXIE;
		put_status_CPUART = FALSE;
	}
	else
	{
		UCA0TXBUF = ch;
		UCA0IE |= UCTXIE;
		put_status_CPUART = TRUE;
	}
}


// Blink LED

#include "Sunseeker2024.h"

/**
 * blink.c
 */

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;       // stop watchdog timer

    P1DIR |= LEDG;                   // configure P1.0 as output
    //P2DIR |= LEDY0 | LEDY1 | LEDG | LEDR;
    //P2OUT |= LEDG;
    //P2OUT &= ~(LEDY0 | LEDY1);

    volatile unsigned int i;        // volatile to prevent optimization

    while(1)
    {
        P1OUT ^= LEDG;              // toggle P1.0
        for(i=10000; i>0; i--);     // delay
    }
}

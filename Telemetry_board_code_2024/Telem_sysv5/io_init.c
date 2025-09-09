/*
 *  I/O Initialization
 *  modified by Drew April 2024
 */

// Include files
#include <msp430fr2476.h>
#include "Sunseeker2024.h"

/*
 * Initialize I/O port directions and states
 * Drive unused pins as outputs to avoid floating inputs and wasting power
 *
 */

void io_init(void)
{
    /******************************PORT 1**************************************/
    P1OUT = 0x00;                                                             // Pull pins low, only affects ports set as output, no effect on inputs
    P1DIR = LED | CAN_SCLK | CAN_SIMO | RS232_TX | P1_UNUSED;                  // Set to output
//    P1OUT = LED | CAN_SIMO | RS232_TX;
    delay();
    P1SEL0 = CAN_SCLK | CAN_SIMO | CAN_SOMI | RS232_TX | RS232_RX;
    P1SEL1 = 0x00;
    //P1DIR &= ~(CAN_S0MI | RS232_RX);                                           // Set to input
        /*Interrupts Enable*/
    P1IFG = 0x00;                                                              // Clear all interrupt flags on Port 1
    delay();

    /******************************PORT 2**************************************/
    P2OUT = 0x00;                                                               // Pull pins low, only affects ports set as output, no effect on inputs
    P2DIR = XT1OUT | LEDY0 | LEDY1 | LEDG | LEDR | P2_UNUSED;                   // Set to output
    P2OUT = XT1OUT | LEDY0 | LEDY1 | LEDG | LEDR;
    P2SEL0 = XT1OUT | XT1IN;
    P2DIR &= ~(XT1IN);                                                          // set to input

    /******************************PORT 3**************************************/
    P3OUT = 0x00;                                                               // Pull pins low, only affects ports set as output, no effect on inputs
    P3DIR = CAN_CSn | CAN_RSTn | P3_UNUSED;                                     // Set to output
    P3OUT |= CAN_RSTn | CAN_CSn;
    delay();
    P3OUT &~ CAN_RSTn;
    delay();
    P3OUT |= CAN_RSTn;
    //P3OUT |= CAN0_SCLK | CAN0_MOSI | SDC_SCLK | SDC_SIMO | CAN1_SCLK;               // Pull used output pins high

    /******************************PORT 4**************************************/
    P4OUT = 0x00;                                                       // Pull pins low, only affects ports set as output, no effect on inputs
    P4DIR = P4_UNUSED;

    /******************************PORT 5**************************************/
    P5OUT = 0x00;                                                               // Pull pins low, only affects ports set as output, no effect on inputs
    P5DIR = P5_UNUSED;

    /******************************PORT 6**************************************/
    P6OUT = 0x00;                                                               // Pull pins low, only affects ports set as output, no effect on inputs
    P6DIR = P6_UNUSED;

}

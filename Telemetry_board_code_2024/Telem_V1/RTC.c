//******************************************************************************
//  MSP430FR267x Demo - RTC, toggle P1.0 every 1s
//
//  Description: Configure ACLK to use 32kHz crystal as RTC source clock,
//               ACLK = XT1 = 32kHz, MCLK = SMCLK = default DCODIV = ~1MHz.
//
//           MSP430FR2676
//         ---------------
//     /|\|               |
//      | |      XIN(P2.1)|--
//      --|RST            |  ~32768Hz
//        |     XOUT(P2.0)|--
//        |               |
//        |          P1.0 |---> LED
//        |               |
//
//   Longyu Fang
//   Texas Instruments Inc.
//   August 2018
//   Built with IAR Embedded Workbench v7.12.1 & Code Composer Studio v8.1.0
//******************************************************************************
#include "Sunseeker2024.h"

void init_RTC(void)
{
    //WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    P2SEL0 |= XT1OUT | XT1IN;                  // P2.0~P2.1: crystal pins
    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);      // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);               // Test oscillator fault flag

    P1OUT &= ~XT1OUT;                         // Clear P1.0 output latch for a defined power-on state
    P1DIR |= XT1OUT;                          // Set P1.0 to output direction

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings


    // RTC count re-load compare value at 32.
    // 1024/32768 * 32 = 1 sec.
    RTCMOD = 32-1;
                                            // Initialize RTC
    // Source = 32kHz crystal, divided by 1024
    RTCCTL = RTCSS__XT1CLK | RTCSR | RTCPS__1024 | RTCIE;

    //__bis_SR_register(LPM3_bits | GIE);     // Enter LPM3, enable interrupt
}

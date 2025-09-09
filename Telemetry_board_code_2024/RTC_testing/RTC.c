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

init_RTC(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings


    // RTC count re-load compare value at 32.
    // 1024/32768 * 32 = 1 sec.
    RTCMOD = 32-1;
                                            // Initialize RTC
    // Source = 32kHz crystal, divided by 1024
    RTCCTL = RTCSS__XT1CLK | RTCSR | RTCPS__1024 | RTCIE;


}
/*
// RTC interrupt service routine
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    switch(__even_in_range(RTCIV,RTCIV_RTCIF))
    {
        case  RTCIV_NONE:   break;          // No interrupt
        case  RTCIV_RTCIF:                  // RTC Overflow
            P1OUT ^= LED;
            break;
        default: break;
    }
}
*/

#include <msp430.h>

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    P2SEL0 |= BIT0 | BIT1;                  // P2.0~P2.1: crystal pins
    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);      // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);               // Test oscillator fault flag

    P1OUT &= ~BIT0;                         // Clear P1.0 output latch for a defined power-on state
    P1DIR |= BIT0;                          // Set P1.0 to output direction

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings


    // RTC count re-load compare value at 32.
    // 1024/32768 * 32 = 1 sec.
    RTCMOD = 256-1;
                                            // Initialize RTC
    // Source = 32kHz crystal, divided by 1024
    RTCCTL = RTCSS__SMCLK | RTCSR | RTCPS__1024 | RTCIE;

    __bis_SR_register(LPM3_bits | GIE);     // Enter LPM3, enable interrupt
}

// RTC interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(RTC_VECTOR))) RTC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(RTCIV,RTCIV_RTCIF))
    {
        case  RTCIV_NONE:   break;          // No interrupt
        case  RTCIV_RTCIF:                  // RTC Overflow
            P1OUT ^= BIT0;
            break;
        default: break;
    }
}

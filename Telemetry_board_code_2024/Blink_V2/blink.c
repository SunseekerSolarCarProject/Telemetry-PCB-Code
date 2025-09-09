//***************************************************************************************
//  MSP430 Blink the LED Demo - Software Toggle P1.0
//
//  Description; Toggle P1.0 by xor'ing P1.0 inside of a software loop.
//  ACLK = n/a, MCLK = SMCLK = default DCO
//
//                MSP430x5xx
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |             P1.0|-->LED
//
//  Texas Instruments, Inc
//  July 2013
//***************************************************************************************

#include <Sunseeker2024.h>

volatile unsigned char status_flag = FALSE;     //status flag set on timer B
volatile unsigned char qsec_flag = FALSE;       //status flag set on timer B
volatile unsigned char cancomm_flag = FALSE;    //status flag set on timer B
volatile char count = 0x00;

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    _DINT();
    _EINT();
    P1DIR |= LED;                          // Set P1.0 to output direction

    clock_init();
    P1OUT ^= LED;
    delay();

    timerB_init();

    for(;;) {
        if (status_flag == TRUE)
        {
        P1OUT ^= LED;                      // Toggle P1.0 using exclusive-OR
        status_flag = FALSE;
        }
    }
}


void timerB_init()
{
    TB0CTL = CNTL_0 | TBSSEL_2 | ID_3 | TBCLR;    // ACLK/8, clear TBR
    TBCCR0 = (SMCLK_RATE/8/TICK_RATE);             // Set timer to count to this value = TICK_RATE overflow
    TBCCTL0 = CCIE;                               // Enable CCR0 interrupt
    TB0CTL |= MC_1;                               // Set timer to 'up' count mode
}

/*
* Timer B CCR0 Interrupt Service Routine
*   - Interrupts on Timer B CCR0 match at 10Hz
*   - Sets Time_Flag variable
*/
/*
* GNU interrupt symantics
* interrupt(TIMER0_B0_VECTOR) timer_b0(void)
*/
#pragma vector = TIMER0_B0_VECTOR
__interrupt void timer_b0(void)
{
    static unsigned int status_count = TELEM_STATUS_COUNT;
    //static unsigned int cancomm_count = CAN_COMMS_COUNT;
    static unsigned int qsec_count = QSEC_COUNT;

    // Primary System Heartbeat
    status_count--;
    if( status_count == 0 )
    {
        status_count = TELEM_STATUS_COUNT;
        status_flag = TRUE;
    }

    qsec_count--;
    if( qsec_count == 0 )
    {
        qsec_count = QSEC_COUNT;
        qsec_flag = TRUE;
    }
/*
    // Periodic CAN Satus Transmission
    if(send_can) cancomm_count--;
    if( cancomm_count == 0 )
    {
        cancomm_count = CAN_COMMS_COUNT;
        cancomm_flag = TRUE;
    }*/
}


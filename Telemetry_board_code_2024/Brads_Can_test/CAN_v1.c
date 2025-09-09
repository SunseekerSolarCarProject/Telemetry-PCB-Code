//***************************************************************************************
//  MSP430 Blink the LED Demo - Software Toggle P1.0
//
//
//  April 2024
//***************************************************************************************

#include <Sunseeker2024.h>
#include "clock_init.h"
#include "can.h"

// Main code parameters
volatile unsigned char status_flag = FALSE;     //status flag set on timer B
volatile unsigned char cancomm_flag = FALSE;     //status flag set on timer B


// CAN Communication Variables
volatile unsigned char send_can = FALSE;       //used for CAN transmission timing
volatile unsigned char rcv_can = FALSE;       //used for CAN transmission timing
volatile unsigned char comms_event_count = 0;
volatile int can_DC_rcv_count, old_DC_rcv_count;

volatile unsigned int can_err_cnt0 = 0x0000;
volatile unsigned int can_err_cnt1 = 0x0000;
volatile unsigned int can_err_cnt23 = 0x0000;
volatile int cancheck_flag;
volatile unsigned char  can_CANINTF, can_FLAGS[3];

volatile int switches_out_dif, switches_dif_save;
volatile int switches_out_new, switches_new;



void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    _DINT();
    P1DIR |= LED;                          // Set P1.0 to output direction

    clock_init();
    P1OUT ^= LED;
    delay();

    io_init();
    P1OUT ^= LED;
    delay();

    timerB_init();
    P1OUT ^= LED;
    delay();

    canspi_init();
    can_init();
    P1OUT ^= LED;
    delay();

    can_DC_rcv_count = 0;
    old_DC_rcv_count = 0;

    P2OUT &= ~(LEDG | LEDR | LEDY0 | LEDY1);



    TB0CCTL0 = CCIE; // Enable CCR0 interrupt
    WDTCTL = WDT_ARST_1000;     // Start watchdog timer to prevent time out reset

    _EINT();


    while(1)
    {
        if(status_flag == TRUE)
        {
        status_flag = FALSE;
        P1OUT ^= LED;                      // Toggle P1.0 using exclusive-OR

        send_can = TRUE;
        rcv_can = TRUE;

        if (cancomm_flag && send_can)
                {
                    cancomm_flag = FALSE;
                    P2OUT ^= LEDY1;

                // Transmit CAN message
                // Transmit Max Cell Voltage
                    can.address = AC_CAN_BASE + BP_VMAX;
                    can.data.data_fp[1] = 0;
                    can.data.data_fp[0] = (float) 0;
                    can_transmit();

                // Transmit Min Cell Voltage
                    can.address = AC_CAN_BASE + BP_VMIN;
                    can.data.data_fp[1] = 0;
                    can.data.data_fp[0] = (float) 0;
                    can_transmit();

                // Transmit Max Cell Temperature
                    can.address = AC_CAN_BASE + BP_TMAX;
                    can.data.data_fp[1] = 0;
                    can.data.data_fp[0] = (float) 0;
                    can_transmit();

                // Transmit Shunt Cutrrent
                    can.address = AC_CAN_BASE + BP_ISH;
                    can.data.data_fp[1] = 0;
                    can.data.data_fp[0] = 0;
                    can_transmit();

                // Transmit our ID frame at a slower rate (every 10 events = 1/second)
                    comms_event_count++;
                    if(comms_event_count >=10)
                    {
                        comms_event_count = 0;
                        can.address = AC_CAN_BASE;
                        can.data.data_u8[7] = 'B';
                        can.data.data_u8[6] = 'P';
                        can.data.data_u8[5] = 'v';
                        can.data.data_u8[4] = '1';
                        can.data.data_u32[0] = DEVICE_SERIAL;
                        cancheck_flag = can_transmit();
                        if(cancheck_flag == 1) can_init();
                    }
                }

        }

        if(((P3IN & CAN_INTn) == 0x00) && rcv_can)
        {
            P2OUT ^= LEDY0;
        // IRQ flag is set, so run the receive routine to either get the message, or the error
            can_receive();
        // Check the status
        // Modification: case based updating of actual current and velocity added
        // - messages received at 5 times per second 16/(2*5) = 1.6 sec smoothing
            if(can.status == CAN_OK)
            {
                switch(can.address)
                {
                case DC_CAN_BASE + DC_SWITCH:
                    switches_out_dif  = can.data.data_u16[3];
                    switches_dif_save = can.data.data_u16[2];
                    switches_out_new  = can.data.data_u16[1];
                    switches_new      = can.data.data_u16[0];

                    // send_or_can = TRUE; // ensbale send once a receive is competed
                    // Add DC Charge mode here
                    // ...
                    can_DC_rcv_count++;
                    break;
                default:
                    break;
                }

             P2OUT &= ~(LEDR);


            }
            else if(can.status == CAN_RTR)
            {
                switch(can.address)
                {
                    case BP_CAN_BASE:
                        can.address = BP_CAN_BASE;
                        can.data.data_u8[7] = 'B';
                        can.data.data_u8[6] = 'P';
                        can.data.data_u8[5] = 'v';
                        can.data.data_u8[4] = '1';
                        can.data.data_u32[0] = DEVICE_SERIAL;
                        can_transmit();
                        break;
                    case BP_CAN_BASE + BP_VMAX:
                        can.data.data_fp[1] = 0;
                        can.data.data_fp[0] = (float) 0;
                        can_transmit();
                        break;
                    case BP_CAN_BASE + BP_VMIN:
                        can.data.data_fp[1] = 0;
                        can.data.data_fp[0] = (float) 0;
                        can_transmit();
                        break;
                    case BP_CAN_BASE + BP_TMAX:
                        can.data.data_fp[1] = 0;
                        can.data.data_fp[0] = (float) 0;
                        can_transmit();
                        break;
                    case BP_CAN_BASE + BP_ISH:
                        can.data.data_fp[1] = 0;
                        can.data.data_fp[0] = (float) 0;
                        can_transmit();
                        break;
                    case BP_CAN_BASE + BP_PCDONE:
                        {
                            can.data.data_u8[7] = 'B';
                            can.data.data_u8[6] = 'P';
                            can.data.data_u8[5] = 'N';
                            can.data.data_u8[4] = 'P';
                            can.data.data_u32[0] = DEVICE_SERIAL;
                        }
                        can_transmit();
                        break;
                }
            }
            else if(can.status == CAN_FERROR)
            {
                P2OUT ^= LEDR;
                can_err_cnt0++;
            }
            else if(can.status == CAN_ERROR)
            {
                P2OUT ^= LEDR;
                can_err_cnt1++;
            }
            else if(can.status == CAN_MERROR)
            {
                P2OUT ^= LEDR;
                can_err_cnt23++;
            }
        }
        WDTCTL = WDT_ARST_1000; // Stop watchdog timer to prevent time out reset

    }
}

/*
* Initialise Timer B
*   - Provides timer tick timebase at 100 Hz
*/
void timerB_init( void )
{
    TB0CTL = CNTL_0 | TBSSEL_1 | ID_3 | TBCLR;     // ACLK/8, clear TBR
    TB0CCR0 = (ACLK_RATE/8/TICK_RATE);             // Set timer to count to this value = TICK_RATE overflow
//    TB0CTL = CNTL_0 | TBSSEL_2 | ID_3 | TBCLR;     // SMCLK/8, clear TBR
//    TB0CCR0 = (SMCLK_RATE/8/TICK_RATE);             // Set timer to count to this value = TICK_RATE overflow
//  TBCCTL0 = CCIE;                               // Enable CCR0 interrrupt
    TB0CTL |= MC_1;                                // Set timer to 'up' count mode
}

/*
* Timer B CCR0 Interrupt Service Routine
*   - Interrupts on Timer B CCR0 match at 10Hz
*   - Sets Time_Flag variable
*/
/*
* GNU interropt symantics
* interrupt(TIMERB0_VECTOR) timer_b0(void)
*/
#pragma vector = TIMERB0_VECTOR
__interrupt void timer_b0(void)
{
    static unsigned int status_count = STATUS_COUNT;
    static unsigned int cancomm_count = CAN_COMMS_COUNT;

    // Primary System Heartbeat
    status_count--;
    if( status_count == 0 )
    {
        status_count = STATUS_COUNT;
        status_flag = TRUE;
    }

    // Periodic CAN Satus Transmission
    if(send_can) cancomm_count--;
    if( cancomm_count == 0 )
    {
        cancomm_count = CAN_COMMS_COUNT;
        cancomm_flag = TRUE;
    }
}


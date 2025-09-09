//***************************************************************************************
//  MSP430 Telemetry data sending out information to a modem.
//
//
//  April 2024
//***************************************************************************************

#include <msp430fr2476.h>
#include "Sunseeker2024.h"
#include "can.h"
#include "UART.h"
#include "decode_packet.h"
#include "can_FIFO.h"

// structures
can_message_fifo can_queue;
//char_fifo USB_FIFO, MODEM_FIFO;

can_struct TX_can_message;
//can_struct can_MPPT;

hf_packet pckHF;
lf_packet pckLF;
status_packet pckST;

enum MODE {INIT, DECODE, MODEMTX, LOWP, LOOP} ucMODE;
char ucFLAG;

// Main code parameters
volatile unsigned char status_flag = FALSE;     //status flag set on timer B
volatile unsigned char cancomm_flag = FALSE;     //status flag set on timer B

// Time and UART parameters
//unsigned int rtcyear = 0, rtcmonth =1, rtcdate = 1;
unsigned char rtchrs= 0, rtcmin = 0, rtcsec = 0; //MCP7940M format
int thrs, tmin, tsec; // BCD representation

static char time_test_msg[18] = "TL_TIM,HH:MM:SS\r\n\0";

volatile unsigned char hs_comms_flag = FALSE;
volatile unsigned char ls_comms_flag = FALSE;
volatile unsigned char st_comms_flag = FALSE;

static const char init_msg_data[21] = "0xHHHHHHHH,0xHHHHHHHH";

//Modem Variables
char put_status_CPUART = FALSE;
char end_CPUART_TX = FALSE;
char *CPUART_TX_ptr;

char command[32];                               //stores CPUART commands
unsigned char uart_count = 0;                  //counts characters received
char modem_var1[32] = "var1\r";                 //command to read batt temperatures
char modem_var1_status = 0;                     //TRUE if battery temps is sent
char modem_var2[32] = "var2\r";                 //command to read cell volts
char modem_var2_status = 0;                     //TRUE if battery volts is sent

// CAN Communication Variables
volatile unsigned char send_can = FALSE;       //used for CAN transmission timing
volatile unsigned char rcv_can = FALSE;       //used for CAN transmission timing
volatile unsigned char can_full = FALSE;    //used for CAN transmission status
volatile unsigned char can_fifo_full = FALSE;   //used for CAN transmission status
volatile unsigned char comms_event_count = 0;
volatile int can_DC_rcv_count, old_DC_rcv_count;

unsigned volatile char can_status_test, can_rcv_status_test;
unsigned long can_msg_count = 0, can_stall_cnt = 0, can_no_int_cnt=0;
unsigned long can_err_count = 0, can_read_cnt = 0;

char CAN_INT_FLAG = FALSE;

volatile unsigned int can_err_cnt0 = 0x0000;
volatile unsigned int can_err_cnt1 = 0x0000;
volatile unsigned int can_err_cnt23 = 0x0000;
volatile int cancheck_flag;
volatile unsigned char  can_CANINTF, can_FLAGS[3];

volatile int switches_out_dif, switches_dif_save;
volatile int switches_out_new, switches_new;

int ii;

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    _DINT();
    P1DIR |= LED;                          // Set P1.0 to output direction

    ucMODE = INIT;
    ucFLAG = 0x80;

    io_init();
    P1OUT ^= LED;
    delay();

    clock_init();
    P1OUT ^= LED;
    delay();

    timerB_init();
    P1OUT ^= LED;
    delay();

    rtc_init();
    P1OUT ^= LED;
    delay();

    // Prepare and deliver Modem text message
    getRTCTime(&thrs,&tmin,&tsec);
    delay();

    CPUART_init();
    P1OUT ^= LED;
    delay();

    CPUART_puts(MODEMCmd);              // +++ sent to modem
    for (ii=0;ii<0x100;ii++) delay();


    CPUART_puts(RFModemH);              // command mode for modem
    delay();
    CPUART_puts(cr_lf_msg);         // return modem to regular data
    delay();

    CPUART_puts("\n\r\0");              // send text through modem
    CPUART_puts("Telem_Lite\0");
    P2OUT ^= LEDG;
    delay();
    CPUART_puts("Starting\0");
    P2OUT ^= LEDG;

    insert_time(&time_test_msg[0]);
    CPUART_puts(time_test_msg);
    delay();

    canspi_init();
    can_init();
    P2OUT ^= LEDG;
    delay();

    can_DC_rcv_count = 0;
    old_DC_rcv_count = 0;

    P2OUT &= ~(LEDG | LEDR | LEDY0 | LEDY1);

    TB0CCTL0 = CCIE; // Enable CCR0 interrupt
    RTCCTL |= RTCIE;

//    WDTCTL = WDT_ARST_1000;     // Start watchdog timer to prevent time out reset

    _EINT();

    // Init operations
    can_fifo_INIT();

    packet_init();
    pckHF.msg_filled = 0;
    pckLF.msg_filled = 0;
    pckST.msg_filled = 0;

    while(1)
    {
        if(ucMODE != INIT) // check for can bus int flag
        {
             // The following is to recover after a breakpoint
             // Based on timing, it happens periodically.
             /*CAN_INT_FLAG = ((P3IN & CAN_INTn)==0);
             if(CAN_INT_FLAG){
                 CAN_INT_FLAG = FALSE;
                 can_flag_check();  //could read CANSTAT instead
                 if(can_CANINTF& 0x03){
                     can_stall_cnt++;
                     can_receive();
                     can_no_int_cnt++;
                 }
             }*/

             if (can_fifo_STAT(&can_queue)){
                 ucMODE = DECODE;
             }
             else
                  ucMODE = LOOP;
         }

        switch (ucMODE) // Based on the Mode
        {
            case (INIT):
                can_fifo_INIT();
                packet_init();
                pckHF.msg_filled = 0;
                pckLF.msg_filled = 0;
                pckST.msg_filled = 0;

                can_init();

                P3IES = CAN_INTn;
                P3IE  = CAN_INTn;  // Enable can Interrupts

                ucMODE = LOOP;
                WDTCTL = WDT_ARST_1000;     // Start watchdog timer to prevent time out reset
                _EINT();    //enable global interrupts
                __no_operation();           //for compiler

                send_can = TRUE;
                rcv_can = TRUE;

                break;
            case (DECODE):
                 decode();
                 if(can_fifo_STAT(&can_queue)){
                     ucFLAG |= 0x20;  //if queue is not empty then continue to decode
                 }
                 else {
                     ucMODE = LOOP;
                 }
                 break;
            case (MODEMTX):
                 break;
            case (LOOP):
                 break;
            default:
                 break;
        }

        if(can_fifo_full){
            __disable_interrupt();
            P2OUT ^= LEDR;                          // Toggle LED
            can_fifo_INIT();
            __enable_interrupt();
        }

        if(end_CPUART_TX){          // Modem transmission ended, clear data for all messages
            end_CPUART_TX = FALSE;
            pckHF.msg_filled = 0;
            hs_comms_flag = FALSE;
            for(ii =0;ii<HF_MSG_PACKET;ii++){
                strncpy(&pckHF.xmit[ii].message[7],init_msg_data,21);
            }
            pckLF.msg_filled = 0;
            ls_comms_flag = FALSE;
            for(ii =0;ii<LF_MSG_PACKET;ii++){
                strncpy(&pckLF.xmit[ii].message[7],init_msg_data,21);
            }
            // Also reset ST packet so next status cycle sends fresh values
            pckST.msg_filled = 0;
            st_comms_flag = FALSE;
            for(ii =0;ii<ST_MSG_PACKET;ii++){
            strncpy(&pckST.xmit[ii].message[7],init_msg_data,21);
            }
        }
        if(hs_comms_flag && (put_status_CPUART == FALSE)) // Send Text Message to Modem
        {
            hs_comms_flag = FALSE;
            P1OUT ^= LEDR;                      // Toggle P1.0 using exclusive-OR

            getRTCTime(&thrs,&tmin,&tsec);
            delay();
            insert_time(&pckHF.timexmit.time_msg[0]);

            CPUART_TX_ptr = &pckHF.prexmit.pre_msg[0];   // set TX pointer
            CPUART_puts_int();                      // Start int modem TX
        }
        if(ls_comms_flag && (put_status_CPUART == FALSE)) // Send Text Message to Modem
                {
                    ls_comms_flag = FALSE;
                    P1OUT ^= LEDR;                      // Toggle P1.0 using exclusive-OR

                    getRTCTime(&thrs,&tmin,&tsec);
                    delay();
                    insert_time(&pckLF.timexmit.time_msg[0]);

                    CPUART_TX_ptr = &pckLF.prexmit.pre_msg[0];   // set TX pointer
                    CPUART_puts_int();                      // Start int modem TX
                }
        if (st_comms_flag && (put_status_CPUART == FALSE)) {
            st_comms_flag = FALSE;
            P1OUT ^= LEDR;

            getRTCTime(&thrs,&tmin,&tsec);
            delay();
            insert_time(&pckST.timexmit.time_msg[0]);

            CPUART_TX_ptr = &pckST.prexmit.pre_msg[0];
            CPUART_puts_int();
        }

        if (cancomm_flag && send_can)
        {
           cancomm_flag = FALSE;
                //P2OUT ^= LEDR;

           // Transmit our ID frame at a slower rate (every 10 events = 1/second)
           comms_event_count++;
           if(comms_event_count >=10)
           {
               comms_event_count = 0;
               TX_can_message.address = TM_CAN_BASE;
               TX_can_message.data.data_u8[7] = 'T';
               TX_can_message.data.data_u8[6] = 'M';
               TX_can_message.data.data_u8[5] = 'v';
               TX_can_message.data.data_u8[4] = '2';
               TX_can_message.data.data_u32[0] = DEVICE_SERIAL;
               cancheck_flag = can_transmit();
               P2OUT ^= LEDY1;
               if(cancheck_flag == 1) can_init();
           }
        }
          // End periodic Modem Transmission
        WDTCTL = WDT_ARST_1000; // Stop watchdog timer to prevent time out reset
    }   // end while(TRUE)
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
//    TB0CCTL0 = CCIE;                               // Enable CCR0 interrupt
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
    static unsigned int hs_comms_count = HS_COMMS_SPEED;
    //static unsigned int ls_comms_count = LS_COMMS_SPEED;
    //static unsigned int st_comms_count = ST_COMMS_SPEED;

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

    if (ucMODE != INIT)
    {
          // Trigger comms events (hs command packet transmission)
            hs_comms_count--;
            if( hs_comms_count == 0 ){
                hs_comms_count = HS_COMMS_SPEED;
                hs_comms_flag = TRUE;
            }
            /*
            ls_comms_count--;
            if( ls_comms_count == 0 ){
                ls_comms_count = LS_COMMS_SPEED;
                ls_comms_flag = TRUE;
            }

            st_comms_count--;
            if( st_comms_count == 0 ){
                st_comms_count = ST_COMMS_SPEED;
                st_comms_flag = TRUE;
            }*/
    }
}

// RTC interrupt service routine
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    extern unsigned char rtchrs, rtcmin, rtcsec;
    switch(__even_in_range(RTCIV,RTCIV_RTCIF))
    {
        case  RTCIV_NONE:   break;          // No interrupt
        case  RTCIV_RTCIF:                  // RTC Overflow
            P2OUT ^= LEDG;
            rtcsec++;
            if (rtcsec >= 60)
            {
                rtcsec = 0;
                rtcmin++;
            }
            if (rtcmin >= 60)
            {
                rtcmin = 0;
                rtchrs++;
            }
            if (rtchrs >= 24)
            {
                rtchrs = 0;
            }
            break;
        default: break;
   }
}

#pragma vector=PORT3_VECTOR
__interrupt void P3_ISR(void)
{
  //int i;
  switch(__even_in_range(P3IV,16))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector Pin 3.0 - CAN_RSTn
    break;
  case 4:                                   // Vector Pin 3.1 - CAN_RXB0
    break;
  case 6:                                   // Vector Pin 3.2 - CAN_RXB1
    break;
  case 8:                                   // Vector Pin 3.3 - CAN_INTn
  {
      unsigned char flags;
      do {

          P2OUT ^= LEDY0;
          can_receive();
          P3IFG &= ~CAN_INTn;
          can_read(CANINTF, &flags, 1);
      }while (flags & (MCP_IRQ_RXB0 | MCP_IRQ_RXB1 | MCP_IRQ_ERR | MCP_IRQ_MERR));
      //can_stall_cnt++;
      //can_receive();
      //P2OUT ^= LEDY0;
      //P3IFG &= 0xF7;
    break;
  }
  case 10:                                  // Vector Pin 3.4 - UNUSED
    break;
  case 12:                                  // Vector Pin 3.5 - UNUSED
    break;
  case 14:                                  // Vector Pin 3.6 - UNUSED
    break;
  case 16:                                  // Vector Pin 3.7 - CAN_CS
    break;
  default:
    break;
  }
}


//UART Interrupt
#pragma vector = USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    extern char *CPUART_TX_ptr;
    extern char put_status_CPUART;
    char ch;
    int ii;

    switch(__even_in_range(UCA0IV,16))
    {
      case 0:break;                             // no interrupt
      case 2:                                   // UCRXIFG
        command[uart_count] = CPUART_getchar();        //get character
        //CPUART_putchar(command[uart_count]);     // put char
        if(command[uart_count] == 0x0D)                //if return
        {
            uart_count = 0x00;                         //reset counter
            if(strcmp(modem_var1,command) == 0) modem_var1_status = 1;
            else if(strcmp(modem_var2,command) == 0) modem_var2_status = 1;
            else
            {
                modem_var1_status = 0;
                modem_var2_status = 0;
            }

            for(ii = 0; ii < 31; ii++)
            {
                command[ii] = 0;                            //reset command
            }
//          CPUART_putchar(0x0A);                       //new line
//          CPUART_putchar(0x0D);                       //return
        }
        else if(command[uart_count] != 0x7F)           //if not backspace
        {
            uart_count++;
        }
        else
        {
            uart_count--;
        }
        break;
      case 4:                                   // UCTXIFG
        ch = *CPUART_TX_ptr++;

        if (ch == '\0')
        {
            UCA0IE &= ~UCTXIE;
            put_status_CPUART = FALSE;
            end_CPUART_TX = TRUE;
        }
        else
        {
            UCA0TXBUF = ch;
            UCA0IE |= UCTXIE;
            put_status_CPUART = TRUE;
        }
        break;
    default:
    break;
      }
}

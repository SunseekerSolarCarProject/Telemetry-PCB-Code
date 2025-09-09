//
// Telemetry
//
/* New doe 2021 B. Bazuin
 * - Can0 functional
 * - UART - RS232 functional (may be RX TX reversed?)
 * - RTC IC function with polling (jumper wire no battery)
 *
 * - Operational to perform required functions for Telemetry in 2021
 * - can is placed in a 16 message queue
 * - can from queue decoded into telemetry messages
 * - messages are periodically transmitted as commanded
 * - interrupt driven UART transmission
 *
 */

#include "Sunseeker2021.h"

// structures
can_message_fifo can0_queue;
//char_fifo USB_FIFO, MODEM_FIFO;

can_struct TX_can0_message;

hf_packet pckHF;
lf_packet pckLF;
status_packet pckST;

unsigned volatile char forceread;

unsigned int can_mask0, can_mask1;
unsigned char can_CANINTF, can0_FLAGS[3], can1_FLAGS[3];

enum MODE {INIT, CANREAD, DECODE, MODEMTX, USBTX, LOWP, LOOP} ucMODE;
unsigned volatile char can_status_test, can_rcv_status_test;
unsigned long can_msg_count = 0, can_stall_cnt = 0, can_no_int_cnt=0;
unsigned long can_err_count = 0, can_read_cnt = 0;

char CAN0_INT_FLAG = FALSE;
char CAN1_INT_FLAG = FALSE;

unsigned char byear, bmonth, bdate, bwkday, bhrs, bmin, bsec; //MCP7940M format
int thrs, tmin, tsec; // BCD representation
char ucFLAG,usbENABLE;

volatile unsigned char status_flag = FALSE;
volatile unsigned char hs_comms_flag = FALSE;
volatile unsigned char ls_comms_flag = FALSE;
volatile unsigned char st_comms_flag = FALSE;

static char init_msg_data[21] = "0xHHHHHHHH,0xHHHHHHHH";
static char init_time_msg[17] = "TL_TIM,HH:MM:SS\r\n";

char time_test_msg[18] = "TL_TIM,HH:MM:SS\r\n\0";
char cr_lf_msg[3] = "\r\n\0";


// General Variables

// CAN Communication Variables
volatile unsigned char cancomm_flag = FALSE;	//used for CAN transmission timing
volatile unsigned char send_can = FALSE;	//used for CAN transmission timing
volatile unsigned char rcv_can = FALSE;	//used for CAN transmission timing
volatile unsigned char can_full = FALSE;	//used for CAN transmission status
volatile unsigned char can_fifo_full = FALSE;	//used for CAN transmission status
int can_comms_event_count = 0;

//Modem_RS232 Variables
char put_status_MODEM = FALSE;
char end_Modem_TX = FALSE;
char *Modem_TX_ptr;

char command[32];								//stores rs232 commands
char buff[32];									//buff array to hold sprintf string
unsigned char modem_count = 0;					//counts characters received
char modem_var1[32] = "var1\r";					//command to read batt temperatures
char modem_var1_status = 0;						//TRUE if battery temps is sent
char modem_var2[32] = "var2\r";					//command to read cell volts
char modem_var2_status = 0;						//TRUE if battery volts is sent

int main(void) {
	int ii;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	_DINT();     		    	//disables interrupts

	ucMODE = INIT;
	ucFLAG = 0x80;

	clock_init();								//Configure HF and LF clocks
	delay();

	io_init();
	delay();

	timerB_init();	//init timer B
	delay();

	// Setup on-chip RTC
	init_RTC();
	delay();

	// Setup RS232 port and Modem
	Modem_UART_init();
	delay();

    MODEM_command_puts(RFModemH);
    Modem_UART_puts(cr_lf_msg);
	delay();

	// Prepare and deliver Modem tets message
	getRTCTime(&thrs,&tmin,&tsec);
	delay();

	insert_time(&time_test_msg[0]);
	Modem_UART_puts(time_test_msg);
	delay();

	// MSP430 RTC init
	init_i2c();
	delay();

	init_MCP7940M();
	delay();

	get_MCP7940M(&bhrs,&bmin,&bsec);
	delay();
	insert_time_2(&time_test_msg[0]);
	Modem_UART_puts(time_test_msg);
	delay();

	get_MCP7940M(&bhrs,&bmin,&bsec);
	delay();
	insert_time_2(&time_test_msg[0]);
	Modem_UART_puts(time_test_msg);
	delay();

	get_MCP7940M(&bhrs,&bmin,&bsec);
	delay();
	insert_time_2(&time_test_msg[0]);
	Modem_UART_puts(time_test_msg);
	delay();

	get_MCP7940M(&bhrs,&bmin,&bsec);
	delay();
	insert_time_2(&time_test_msg[0]);
	Modem_UART_puts(time_test_msg);
	delay();

	// Set up both Can Controllers
	can0spi_init();
	delay();

	can0_init();
	delay();

	can1spi_init();
	delay();

	can1_init();
	delay();

    while(1)
    {
    	if(ucMODE != INIT){
            // The following is to recover after a breakpoint
    		// Based on timing, it happens periodically.
            CAN0_INT_FLAG = ((P2IN & CAN0_INTn)==0);
            CAN1_INT_FLAG = ((P2IN & CAN1_INTn)==0);

            if(CAN0_INT_FLAG){
            	CAN0_INT_FLAG = FALSE;
            	can0_flag_check();	//could read CANSTAT instead
            	if(can_CANINTF& 0x03){
            		can_stall_cnt++;
            		can0_receive();
            		can_no_int_cnt++;
            	}
            }
                if(CAN1_INT_FLAG){
                	CAN1_INT_FLAG = FALSE;
                	can1_flag_check();	//could read CANSTAT instead
                	if(can_CANINTF& 0x03){
                		can_stall_cnt++;
                		can1_receive();
                		can_no_int_cnt++;
                	}
            }

            if (can_fifo_STAT(&can0_queue)){
        		ucMODE = DECODE;
            }
            else
            	 ucMODE = LOOP;

    	}

        switch (ucMODE)
        {
          case (INIT):
		    can_fifo_INIT();
            packet_init();
            pckHF.msg_filled = 0;
            pckLF.msg_filled = 0;
            pckST.msg_filled = 0;


            can0_init();
            can1_init();

            P2IES = CAN0_INTn;
            P2IE  = CAN0_INTn;	// Enable can0 Interrupts
            P2IES = CAN1_INTn;
            P2IE  = CAN1_INTn;	// Enable can1 Interrupts

		    ucMODE = LOOP;
		    WDTCTL = WDT_ARST_1000; 	// Start watchdog timer to prevent time out reset
			_EINT(); 	//enable global interrupts
			__no_operation();			//for compiler

		    break;
          case (CANREAD):
    	    can_stall_cnt++;
    	    can1_receive();
		    break;
          case (DECODE):
		    decode();
            if(can_fifo_STAT(&can0_queue)){
            	ucFLAG |= 0x20;  //if queue is not empty then continue to decode
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
    		P8OUT ^= BIT4;                          // Toggle LED
    		can_fifo_INIT();
        }

        if(end_Modem_TX){
        	end_Modem_TX = FALSE;
            pckHF.msg_filled = 0;
            hs_comms_flag = FALSE;
            for(ii =0;ii<HF_MSG_PACKET;ii++){
            	strncpy(&pckHF.xmit[ii].message[7],init_msg_data,21);
            }

        }

    	if(status_flag){
    		status_flag = FALSE;
    		P8OUT ^= BIT3;                          // Toggle LED
    		P8OUT ^= BIT6;                          // Toggle LED

    		get_MCP7940M(&bhrs,&bmin,&bsec);
    		insert_time_2(&pckHF.timexmit.time_msg[0]);

        	//getRTCTime(&thrs,&tmin,&tsec);
            //insert_time(&pckHF.timexmit.time_msg[0]);

            Modem_TX_ptr = &pckHF.prexmit.pre_msg[0]; 	// set TX pointer
            Modem_UART_puts_int();						// Start int modem TX

   			// Transmit our ID frame at a slower rate (every 10 events = 1/second)
    		can_comms_event_count++;
    		if(can_comms_event_count >=0)
    		{
    			can_comms_event_count = 0;
    			TX_can0_message.address = TM_CAN_BASE;
    			TX_can0_message.data.data_u8[7] = 'T';
    			TX_can0_message.data.data_u8[6] = 'M';
    			TX_can0_message.data.data_u8[5] = 'v';
    			TX_can0_message.data.data_u8[4] = '1';
    			TX_can0_message.data.data_u32[0] = DEVICE_SERIAL;
    			can1_transmit();
   			}

    	}  // End periodic communications
    	WDTCTL = WDT_ARST_1000; // Reset watchdog timer to prevent time out reset

    }  // end while(TRUE)

	return 0;
}

/*
* Initialise Timer B
*	- Provides timer tick timebase at 100 Hz
*/
void timerB_init( void )
{
  TBCTL = CNTL_0 | TBSSEL_1 | ID_3 | TBCLR;		// ACLK/8, clear TBR
  TBCCR0 = (ACLK_RATE/8/TICK_RATE);				// Set timer to count to this value = TICK_RATE overflow
  TBCCTL0 = CCIE;								// Enable CCR0 interrrupt
  TBCTL |= MC_1;								// Set timer to 'up' count mode
}

/*
* Timer B CCR0 Interrupt Service Routine
*	- Interrupts on Timer B CCR0 match at 10Hz
*	- Sets Time_Flag variable
*/
/*
* GNU interropt symantics
* interrupt(TIMERB0_VECTOR) timer_b0(void)
*/
#pragma vector = TIMER0_B0_VECTOR
__interrupt void timer_b0(void)
{
	  static unsigned int status_count = TELEM_STATUS_COUNT;
	  static unsigned int hs_comms_count = HS_COMMS_SPEED;
	  static unsigned int ls_comms_count = LS_COMMS_SPEED;
	  static unsigned int st_comms_count = ST_COMMS_SPEED;

	  if (ucMODE != INIT)
	  {
	  // Trigger comms events (hs command packet transmission)
	  	hs_comms_count--;
	  	if( hs_comms_count == 0 ){
	   		hs_comms_count = HS_COMMS_SPEED;
	    	hs_comms_flag = TRUE;
	  	}

	  	ls_comms_count--;
	  	if( ls_comms_count == 0 ){
	    	ls_comms_count = LS_COMMS_SPEED;
	    	ls_comms_flag = TRUE;
	  	}

	  	st_comms_count--;
	  	if( st_comms_count == 0 ){
	    	st_comms_count = ST_COMMS_SPEED;
	    	st_comms_flag = TRUE;
	  	}
	  }

	  // Primary System Heart beat
	  status_count--;
	  if( status_count == 0 )
	  {
		  status_count = TELEM_STATUS_COUNT;
		  status_flag = TRUE;
	  }


}

//------------------------------------------------------------------------------
// The i2c_B1TX_isr is structured such that it can be used to transmit any
// number of bytes by pre-loading TXByteCtr with the byte count. Also, TXData
// points to the next byte to transmit.
//------------------------------------------------------------------------------
/*
* I2B UCB1 Interrupt Service Routine
*/
/*
* GNU interrupt symantics
* interrupt(UCB1IV) i2c_B1_isr(void)
*/
#pragma vector = USCI_B1_VECTOR
__interrupt void i2c_B1_isr(void)
{
	volatile char textcnt;

	  switch(__even_in_range(UCB1IV,16))
	  {
	  case 0:break;                             // no interrupt
	  case 2:                                   // UCRXIFG
	    break;
	  case 4:                                   // UCTXIFG
		break;
      default:
    	break;
	  }

}

#pragma vector=PORT2_VECTOR
__interrupt void P2_ISR(void)
{
  //int i;
  switch(__even_in_range(P2IV,16))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector Pin 2.0 - CAN0_INTn
    can_stall_cnt++;
    can0_receive();
    P2IFG &= 0xFE;
    break;
  case 4:                                   // Vector Pin 2.1 - CAN0_RXB0n
    break;
  case 6:                                   // Vector Pin 2.2 - CAN0_RXB1n
    break;
  case 8:                                   // Vector Pin 2.3 - CAN1_INTn
	can_stall_cnt++;
	can1_receive();
 P2IFG &= 0xF7;
    break;
  case 10:                                  // Vector Pin 2.4 - CAN1_RXB0n
    break;
  case 12:                                  // Vector Pin 2.5 - CAN1_RXB1n
    break;
  case 14:                                  // Vector Pin 2.6 - GPS_INTn
    break;
  case 16:                                  // Vector Pin 2.7 - UNUSED
    break;
  default:
    break;
  }
}

//RS232 Interrupt
#pragma vector = USCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
    extern char *Modem_TX_ptr;
    extern char put_status_MODEM;
	char ch;
	int ii;

	switch(__even_in_range(UCA3IV,16))
	{
	  case 0:break;                             // no interrupt
	  case 2:                                   // UCRXIFG
		command[modem_count] = Modem_UART_getchar();		//get character
		//Modem_UART_putchar(command[modem_count]);		// put char
		if(command[modem_count] == 0x0D)				//if return
		{
			modem_count = 0x00;							//reset counter
			if(strcmp(modem_var1,command) == 0) modem_var1_status = 1;
			else if(strcmp(modem_var2,command) == 0) modem_var2_status = 1;
			else
			{
				modem_var1_status = 0;
				modem_var2_status = 0;
			}

			for(ii = 0; ii < 31; ii++)
			{
				command[ii] = 0;							//reset command
			}
//			Modem_UART_putchar(0x0A);						//new line
//			Modem_UART_putchar(0x0D);						//return
		}
		else if(command[modem_count] != 0x7F)			//if not backspace
		{
			modem_count++;
		}
		else
		{
			modem_count--;
		}
	    break;
	  case 4:                                   // UCTXIFG
		ch = *Modem_TX_ptr++;

		if (ch == '\0')
		{
			UCA3IE &= ~UCTXIE;
			put_status_MODEM = FALSE;
			end_Modem_TX = TRUE;
		}
		else
		{
			UCA3TXBUF = ch;
			UCA3IE |= UCTXIE;
			put_status_MODEM = TRUE;
		}
	  	break;
    default:
  	break;
	  }


}


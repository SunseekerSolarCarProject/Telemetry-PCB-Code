//
// Telemetry
//
// Modified by Erik in 2010-2011
//
/* Modifications for 2013 by B. Bazuin
 * - 2013v1
 * - reworked to operate continuously with CAN at 250 kbps
 * - no USB device code included
 * - no ADC code
 * - table values collected only the first time captured
 * - 2014V2 MODIFICATIONS
 * - Reordered and eliminated some messages
 * - HF rate 10 sec, LF rate 30 sec, ST rate 60 sec
 * - BP_PCDONE and BP_ISH added
 * - Precharge Controller Removed
 */
/* Modifications for 2016 by B. Bazuin
  * -New BPS and Array Controller
  *
 */


#include "Sunseeker2021.h"

// structures
message_fifo decode_queue;
char_fifo USB_FIFO, MODEM_FIFO;

hf_packet pckHF;
lf_packet pckLF;
status_packet pckST;

unsigned volatile char forceread;

unsigned int can_mask0, can_mask1;

char usb_rx_buffer[16];

enum MODE {INIT, CANREAD, DECODE, MODEMTX, USBTX, LOWP, LOOP} ucMODE;
unsigned volatile char can_status_test, can_rcv_status_test;
unsigned long can_msg_count = 0, can_stall_cnt = 0;
unsigned long can_err_count = 0, can_read_cnt = 0;
int thrs, tmin, tsec;
char ucFLAG,usbENABLE;
char CAN1_INT_FLAG = FALSE;

volatile unsigned char hs_comms_flag = FALSE;
volatile unsigned char ls_comms_flag = FALSE;
volatile unsigned char st_comms_flag = FALSE;

static char init_msg_data[21] = "0xHHHHHHHH,0xHHHHHHHH";
  
int main( void )
{
  int i;

  WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer to prevent time out reset
  _DINT();     		    //disables interrupts
  
  ucMODE = INIT;
  ucFLAG = 0x80;
  
  init_LED();
  init_CAN1_pins();
  P1OUT ^= LED8;
  delay();

  init_CAN2_pins();
  P1OUT ^= LED7;
  delay();

  init_RS_pins();
  P1OUT ^= LED8 + LED7 +LED6;
  delay();

  init_USB_pins();
  P1OUT ^= LED8;
  delay();

  init_JMP();
  P1OUT ^= LED8 + LED7;
  delay();

  init_CAN2Volt();
  P1OUT ^= LED8;
  delay();

  init_CLK();		      //initialize Clock

  P1OUT  ^= (LED5 | LED6 | LED7 | LED8);
  delay();
  delay();


  spi1_init();            //initialize CAN1 SPI interfaces
  delay();
  P1OUT  ^= LED8;              //display spi init
  can1_init();     	      //initialize Primary CAN controller
  delay();
  P1OUT  ^= (LED8 + LED7 );              //display spi init
  init_fast_CLK();       //switch can to 16 MHz
  delay();
  P1OUT  ^= (LED8 );              //display spi init

  P1OUT &= ~(LED5 | LED6 | LED7 | LED8);
  P1OUT ^= (LED5 | LED6 | LED7 | LED8);
  delay();
  delay();
  delay();
  delay();
  init_RTC();
  delay();
  _DINT();     		    //disables interrupts
  delay();
  delay();
  P1OUT ^= (LED5 | LED6 | LED7 | LED8);

  getTime(&thrs,&tmin,&tsec);

  delay();
  MODEM_init();
  delay();

  MODEM_command(RFModemS);
  delay();
  P1OUT &= ~(LED5 | LED6 | LED7 | LED8);


  usbENABLE = FALSE;
  

  while(TRUE)
  {
    P1OUT ^= LED3 + LED5 + LED8;            //display processing
    if((ucFLAG & 0x80) != 0x00){
    	ucMODE = INIT;
    }
    else if((ucFLAG & 0x40) != 0x00){
    	ucMODE = CANREAD;
    }
    else if((ucFLAG & 0x20) != 0x00){
    	ucMODE = DECODE;
    }
    else if((ucFLAG & 0x08) != 0x00){
    	ucMODE = MODEMTX;
    }
    else if((ucFLAG & 0x04) != 0x00){
    	ucMODE = USBTX;
    }
    else {
    	ucMODE = LOOP;
    }
    
    switch (ucMODE)
    {
      case (INIT):
       // Remove CAN 2 initialization and functions
/*        if( CAN2_ENABLE) 
	{
	        spi2_init();
      		can2_init();     	//initialize CAN controller
	}
        else
        P1OUT = 0x1C;              //display spi init
 */      
  		// Modem functions
        //MODEM_high_baud();
        P1OUT = 0x20;              //display modem init
           
 		// Remove USB functions   
/*        USB_init();
        //uart2_init();
        P1OUT = 0x24;             //display usb init
*/
        P1OUT = 0x28;             //display can init
          
        CHAR_FIFO_INIT(MODEM_FIFO);
        message_fifo_INIT();
        packet_init();
        pckHF.msg_filled = 0;
        pckLF.msg_filled = 0;
        pckST.msg_filled = 0;

        can_msg_count = 0;
        can_err_count = 0;
        can_read_cnt = 0;
        P1OUT = 0x3C;             //display can init
        P1OUT |= LED3;            //display processing
        delay();
        timerB_init();
        delay();
        delay();

        ucFLAG &= ~0x80;
        P1OUT &= ~(LED3|LED4|LED5|LED6|LED7|LED8);

        ucMODE = LOOP;
          
  	//start_mask(); //Start CAN masks
        WDTCTL = WDT_ARST_1000; 	// Start watchdog timer to prevent time out reset

        _EINT();   //asm("EINT");	//enable interrupts 

        break;
      case (CANREAD):
        P1OUT ^= LED7;            //display processing
        ucFLAG &= ~0x40;
        if((P4IN & CAN1_CONT_nINT)==0){
//          can1_receive(); //reads can message and adds to queue to be decoded
          //ucFLAG |= 0x20;
        }
        break;
      case (DECODE):
        P1OUT ^= LED4 + LED6;            //display processing
        ucFLAG &= ~0x20;
        decode();
        if(message_fifo_STAT(&decode_queue))
          ucFLAG |= 0x20;  //if queue is not empty then continue to decode
        break;
      case (MODEMTX):
        P1OUT ^= LED5;
        ucFLAG &= ~0x08;
        
        getTime(&thrs,&tmin,&tsec);
         
        if ((pckST.msg_filled == 0x3FFF)&&(st_comms_flag == TRUE))
        {
          //MODEM_command(RFModemS);
          insert_time(&pckST.timexmit.time_msg[0]);
          MODEM_TX_PCK(&pckST.prexmit.pre_msg[0]);
          pckST.msg_filled = 0;
          st_comms_flag = FALSE;
          for(i =0;i<ST_MSG_PACKET;i++){
            strncpy(&pckST.xmit[i].message[7],init_msg_data,21);
          }
        }
        else if ((pckLF.msg_filled == 0x00FF)&&(ls_comms_flag == TRUE))
        {
          //MODEM_command(RFModemL);
          insert_time(&pckLF.timexmit.time_msg[0]);
          MODEM_TX_PCK(&pckLF.prexmit.pre_msg[0]);
          pckLF.msg_filled = 0;
          ls_comms_flag = FALSE;
          for(i =0;i<LF_MSG_PACKET;i++){
            strncpy(&pckLF.xmit[i].message[7],init_msg_data,21);
          }
        }
        else if ((pckHF.msg_filled == 0x03FF)&&(hs_comms_flag == TRUE))
        { 
          //MODEM_command(RFModemH);
          insert_time(&pckHF.timexmit.time_msg[0]);
          MODEM_TX_PCK(&pckHF.prexmit.pre_msg[0]);
          pckHF.msg_filled = 0;
          hs_comms_flag = FALSE;
          for(i =0;i<HF_MSG_PACKET;i++){
            strncpy(&pckHF.xmit[i].message[7],init_msg_data,21);
          }
        }

        break;
      case (USBTX):
        P1OUT ^= LED5;
        ucFLAG &= ~0x04;
//        P10OUT |= USB_nRS;	 //Release VDIP from Reset State
//        UART3_RX(&usb_rx_buffer[0]);
//        UART3_TXN(&usb_rx_buffer[0],16);
//        usbENABLE = TRUE;
        break;
        
      case (LOWP):
        //        while((P2IN & JMP3))
        //        {
        //         for(int i = 0 ;i<0xffff;i++);
        //         P1OUT ^= LED5; 
        //        }
        //ucMODE = CANREAD;
//        __bis_SR_register(LPM3_bits + GIE);
        break;
        
      default:
        // Check to see if an unserviced CAN interrupt remains
        // If so, set canread flag
        CAN1_INT_FLAG = ((P4IN & CAN1_CONT_nINT)==0);
        if(CAN1_INT_FLAG){
//          ucFLAG |= 0x40;
          can_stall_cnt++;
          can1_receive();
        }
      
        if (st_comms_flag == TRUE)
        {
          if(pckST.msg_filled != 0x0000)
          {
            pckST.msg_filled = 0x3FFF;
            ucFLAG |= 0x08;
          }
        }
        if (ls_comms_flag == TRUE)
        {
          if(pckLF.msg_filled != 0x0000)
          {
            pckLF.msg_filled = 0x00FF;
            ucFLAG |= 0x08;
          }
        }
        if (hs_comms_flag == TRUE)
        {
          if(pckHF.msg_filled != 0x0000)
          {
            pckHF.msg_filled = 0x03FF;
            ucFLAG |= 0x08;
          }
        }
        if ((can_msg_count & 0x007F) == 0x0040)
        {
//         ucFLAG |= 0x04;
        }
        break;
    }
    WDTCTL = WDT_ARST_1000; // Stop watchdog timer to prevent time out reset
  }
} 

#pragma vector=PORT2_VECTOR
__interrupt void P2_ISR(void)
{
  //int i;
  switch(__even_in_range(P2IV,16))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2.0 - CAN1_nINT
//    ucFLAG |= 0x40;
    can1_receive();     //reads can message and adds to queue to be decoded
//    __low_power_mode_off_on_exit();
    P2IFG &= 0xFE;
    break;
  case 4:                                   // Vector 2.1 - 
    break;                          
  case 6:                                   // Vector 2.2 -  
    break;
  case 8:                                   // Vector 2.3 - 
    break;
  case 10:                                  // Vector 2.4 - 
    break;
  case 12:                                  // Vector 2.5 - 
    break;
  case 14:                                  // Vector 2.6 - 
    break;
  case 16:                                  // Vector 2.7 - 
    break;
  default: 
    break;
  }
}

/*
* Timer B CCR0 Interrupt Service Routine
*	- Interrupts on Timer B CCR0 match at 10Hz
*	- Sets Time_Flag variable
*/
/*
* GNU interrupt semantics
* interrupt(TIMERB0_VECTOR) timer_b0(void)
*/
#pragma vector = TIMERB0_VECTOR  
__interrupt void timer_b0(void) 
{
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
  
}

/*************************************************************
/ Name: init_CAN2Volt
/ IN: void
/ OUT: 1 if successful
/ DESC: This function is used to set up the individual pins for use in 
/ controlling the voltage on the second can bus
************************************************************/
void init_CAN2Volt(void)
{
  P2DIR	|= CAN2_VEN;		//set enable as output
  if(CAN2_ENABLE)
  {
  P2OUT	|= CAN2_VEN;		//active high 
  P2DIR	&= ~(CAN2_VFG);		//set as input
  P2IE	|= CAN2_VFG;		//enable interrupts from voltage Flag
  P2IES	|= CAN2_VFG;		//interrupts occur on high-low transition
  }
  else
  {
  P2OUT	&= ~CAN2_VEN;		//turn off 
  }
}


/*************************************************************
/ Name: init_LED
/ IN: void
/ OUT:  1 if successful
/ DESC:  This function is used to set the pins used to control the LEDs
/ as outputs and to a low value to start
************************************************************/
void init_LED(void)
{
  P1DIR = LED3 | LED4 | LED5 | LED6 | LED7 | LED8;	//set to output
  P1OUT = 0x00;						//start LED's Low
}


/*************************************************************
/ Name: init_JMP
/ IN: void
/ OUT:  1 if successful
/ DESC:  This function is used to set the pins used for the jumpers 
/  and button as inputs and to enable interrupts on a high to low transition
************************************************************/
void init_JMP(void)
{
  P1DIR	&= ~(JMP1 | JMP2);		//set input
  P2DIR	&= ~(JMP3 | BUTTON);	//set input
  P1IE	|= JMP1 | JMP2;			//enable interrupts
  P1IES	|= JMP1 | JMP2;			//high-low transition
  P2IE	|= JMP3 | BUTTON;		//enable interrupts
  P2IES	|= JMP3 | BUTTON;		//high-low transition JMP3 used as CAN1_nINT
}


/*************************************************************
/ Name: init_CAN1_pins
/ IN: void
/ OUT:  1 if successful
/ DESC:  This function is used to set the pins used to control the
/ pins used to control the can transceiver and can controller for
/ appropriate use
************************************************************/
void init_CAN1_pins(void)
{
  P2DIR	|= CAN1_TR_EN | CAN1_TR_RS | CAN1_CONT_nRST;			//sets output
  P2OUT &= ~(CAN1_CONT_nRST);                                           //cause the controller to reset
  P2OUT |= CAN1_TR_EN | CAN1_TR_RS;		        //sets outputs High
  P3DIR	|= CAN1_CONT_nCS | CAN1_SIMO | CAN1_CLK;          		//sets output
  P3DIR &= ~CAN1_SOMI;                                                  //sets as input
  P3OUT	|= CAN1_CONT_nCS;		                                //CS must start high	//sets low
  P3SEL	|= CAN1_SOMI | CAN1_SIMO | CAN1_CLK;				//enable secondary
  P4DIR	&= ~(CAN1_CONT_nINT | CAN1_CONT_nRX0BF | CAN1_CONT_nRX1BF);	//sets input
  P2OUT |= CAN1_CONT_nRST;		        //sets outputs High
}



/*************************************************************
/ Name: init_CAN2_pins
/ IN: void
/ OUT:  1 if successful
/ DESC:  This function is used to set the pins used to control the
/ the can transceiver and can controller for appropriate use
************************************************************/
void init_CAN2_pins(void)
{
  P3DIR	|= CAN2_CONT_nCS | CAN2_SIMO;				                //sets output
  P3OUT |= CAN2_CONT_nCS;
  //P3OUT	&= ~(CAN2_CONT_nCS);				                //sets output low
  P3SEL |= CAN2_SIMO;					                //sets secondary
  P4DIR	&= ~(CAN2_CONT_nINT | CAN2_CONT_nRX0BF | CAN2_CONT_nRX1BF);	//sets input
  P5SEL	|= CAN2_SOMI | CAN2_CLK;			                //sets secondary
  P5DIR |= CAN2_CLK;
  P5DIR &= ~CAN2_SOMI;
  P8DIR	|= CAN2_TR_EN | CAN2_TR_RS | CAN2_CONT_RST;	                //sets output
  P8OUT &= ~(CAN2_CONT_RST);
  P8OUT |= CAN2_TR_EN | CAN2_TR_RS | CAN2_CONT_RST;	                //sets outputs High
}


/*************************************************************
/ Name: init_RS_pins
/ IN: void
/ OUT:  1 if successful
/ DESC:  This function is used to set the pins used to control the
/ the RS232 interface for appropriate use
************************************************************/
void init_RS_pins(void)
{
  P5SEL	|= RS1TX | RS1RX;			//enables secondary for RS232-1
  P8DIR	|= RS_nEN | RS_SHDN;		//sets output
  P8OUT	&= ~(RS_nEN);				//sets low
  P8OUT	|= RS_SHDN;				    //sets high
  P9SEL	|= RS2TX | RS2RX;			//enables secondary for RS232-2
}

/*************************************************************
/ Name: init_USB_pins
/ IN: void
/ OUT:  1 if successful
/ DESC:  This function is used to set the pins used to control the
/ the USB interface for appropriate use
************************************************************/
void init_USB_pins(void)
{
  P6DIR = PWR_5V_EN; 
  P6OUT &= ~(PWR_5V_EN);
 
  P10SEL |= USB_TX | USB_RX;	        	                      //enables secondary
  P10DIR |= USB_nPG | USB_nRS;		                              //sets output
  P10OUT |= USB_nPG | USB_nRS;		                              //sets output high
}

/*
* Initialize Timer B
*	- Provides timer tick timebase at 100 Hz
*/
void timerB_init( void )
{
  TBCTL = CNTL_0 | TBSSEL_1 | ID_3 | TBCLR;		// ACLK/8, clear TBR
  TBCCR0 = (ACLK_RATE/8/TICK_RATE);		// Set timer to count to this value = TICK_RATE overflow
  TBCCTL0 = CCIE;				// Enable CCR0 interrupt
  TBCTL |= MC_1;				// Set timer to 'up' count mode
}


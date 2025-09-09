//
// Telemetry Clock Code
//
#include "Sunseeker2021.h"

  /*************************************************************
/ Name: SetVCoreUp
/ IN: int Level
/ OUT:  void
/ DESC:  This function is used to set the voltage of the VCORE to 
/        The level specified in input
************************************************************/
void SetVCoreUp (unsigned int level)
{
  // Open PMM registers for write access
  PMMCTL0_H = 0xA5;
  // Set SVS/SVM high side new level
  SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
  // Set SVM low side to new level
  SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
  // Wait till SVM is settled
  while ((PMMIFG & SVSMLDLYIFG) == 0);
  // Clear already set flags
  PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
  // Set VCore to new level
  PMMCTL0_L = PMMCOREV0 * level;
  // Wait till new level reached
  if ((PMMIFG & SVMLIFG))
    while ((PMMIFG & SVMLVLRIFG) == 0);
  // Set SVS/SVM low side to new level
  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
  // Wait till SVM is settled
  while ((PMMIFG & SVSMLDLYIFG) == 0);
  // Lock PMM registers for write access
  PMMCTL0_H = 0x00; 
}

/*************************************************************
/ Name: init_CLK
/ IN: void
/ OUT:  void
/ DESC:  This function is used to set inputs and settings of the CLK
************************************************************/
void init_CLK(void)
{
  int i;
	  
  P11DIR |= 0x07;
  P11SEL |= 0x07;
  
  //while(1);
  P5SEL |= XT2IN | XT2OUT;
  P7SEL |= XT1IN | XT1OUT;
  // Set MCLCK to max rate (8MHz)
  SetVCoreUp(1);
  SetVCoreUp(2);
  SetVCoreUp(3);
  //	These may be needed if we choose MCLK/SMCLK to be sourced from DCOCLK instead of DCOCLKDIV
  //	UCSCTL1 |= DCORSEL_7; 		//BCSCTL1 |= 0x07; from msp430f1611
  //								//DCO Freq. Range Select Bits :0,1,2 -- 7
  //	UCSCTL1 |= DISMOD;			//Disable Modulation
  
  //UCSCTL6 setup XT1 as 32.768kHz crystal
  UCSCTL6 &= ~(XT1OFF);
  UCSCTL6 |= XCAP_3 ;		
  UCSCTL6 |= XT1DRIVE_0; 
    
  //UCSCTL2 and UCSCTL3 deal with FLL
  UCSCTL3 |= SELREF_2;
  do
  {
    UCSCTL7 &= ~(XT2OFFG + +XT1LFOFFG + XT1HFOFFG + DCOFFG);    // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                                          // Clear fault flags
    for(i=255;i>0;i--);                                         // Delay for Osc to stabilize
  }while ((SFRIFG1&OFIFG) !=0);                                 // Test oscillator fault flag
  
  //UCSCTL6 setup XT2 as up to 16 MHz external input
  UCSCTL6 |= XT2OFF;
  //UCSCTL6 |= XT2DRIVE_3;
  UCSCTL6 |= XT2BYPASS; //XT2 Sourced Externally from pin - 16MHz

  do
  {
    UCSCTL7 &= ~(XT2OFFG + +XT1LFOFFG + XT1HFOFFG + DCOFFG);   // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                                         // Clear fault flags
    for(i=255;i>0;i--);                                         // Delay for Osc to stabilize
  }while ((SFRIFG1 & OFIFG) != 0x0000);                                // Test oscillator fault flag


 // while(1);  
}

/*************************************************************
/ Name: init_fast_CLK
/ IN: void
/ OUT:  void
/ DESC:  This function is used to increase the clock rate
************************************************************/
void init_fast_CLK()
{
	int i;
  //can_mode(interface, CANCTRL, 0x03, 0x00); // CANCTRL register, modify lower 2 bits, CLK = /4
  can1_canctrl(0x07, 0x05);
  // Delays are required to propagate CAN Clock out to MCLK of MSP430
  delay();
  delay();
  delay();
  delay();

  do
  {
    UCSCTL7 &= ~(XT2OFFG + +XT1LFOFFG + XT1HFOFFG + DCOFFG);   // Clear XT2,XT1,DCO fault flags
    SFRIFG1 &= ~OFIFG;                                         // Clear fault flags
    for(i=255;i>0;i--);                                         // Delay for Osc to stabilize
  }while ((SFRIFG1 & OFIFG) != 0x0000);                                // Test oscillator fault flag
  
  UCSCTL4 |= SELM__XT2CLK;	//Select XT2 as MCLK source 
  UCSCTL4 |= SELA__XT1CLK;	//Select XT1 for ACLK source
  UCSCTL4 |= SELS__XT2CLK;	//Select XT2 as SMCLK source 
  
  UCSCTL5 |= DIVM_0;			//Divide MCLK Source by 1 (MAX) - 16 MHz
  UCSCTL5 |= DIVS_0;			//Divide SMCLK source by 2 (Required to be at least 2) - 8 MHz
  UCSCTL5 |= DIVA_0;			//Divide ACLK source by 1 - 32.768 KHz

}

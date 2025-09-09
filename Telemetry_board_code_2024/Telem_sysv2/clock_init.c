//
//
//
#include <msp430fr2476.h>
#include "Sunseeker2024.h"

void clock_init(void)
{

    char i;

    FRCTL0 = FRCTLPW | NWAITS_1;

    //XT1 CLOCK CONFIG
//    CSCTL6 &= ~(XT1DRIVE);                                          // Enable XT1
    CSCTL6 &= ~(XT1DRIVE1 | XT1DRIVE0 | XTS | XT1BYPASS);                             // Lowest drive current LF 32KHz oscillator

    do{
        CSCTL7 &= ~(XT1OFFG + DCOFFG);                              // Clear XT1,DCO fault flags
        SFRIFG1 &= ~OFIFG;                                          // Clear fault flags
        for(i=255;i>0;i--){                                         // Delay for oscillator to stabilize
            _NOP();                                                 // "No Operation"
        }
    }
    while((SFRIFG1 & OFIFG) != 0);                                  // Test oscillator fault flag

    __bis_SR_register(SCG0);                                        // disable FLL
    CSCTL3 |= SELREF__XT1CLK;                                      // Set REFO as XT1CLK reference source
    CSCTL0 = 0;                                                     // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                                         // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_5;                                            // Set DCO = 16MHz
    CSCTL2 = FLLD_0 + 487;                                          // DCOCLKDIV = 16MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                                               // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));                             // FLL locked

    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;                          // Clock Source ACLK = XT1 = 32kHz
    CSCTL5 |= DIVA_0  | DIVS_2 | DIVM_0;                               // Divide ACLK/1 = 32kHz

}

/*************************************************************
/ Name: SetVCoreUp
/ IN: int Level
/ OUT:  void
/ DESC:  This function is used to set the voltage of the VCORE to
/        The level specified in input
/ Reference: Users Guide page 74
************************************************************/
/*
void SetVCoreUp (unsigned int level)
{
    PMMCTL0_H = 0xA5;                                                   // Open PMM registers for write access
    SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;    // Set SVS/SVM high side new level
    SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;                       // Set SVM low side to new level
    while((PMMIFG & SVSMLDLYIFG) == 0);                                 // Wait till SVM is settled
    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);                                  // Clear already set flags
    PMMCTL0_L = PMMCOREV0 * level;                                      // Set VCore to new level
    if(PMMIFG & SVMLIFG){
        while((PMMIFG & SVMLVLRIFG) == 0);                              // Wait till new level reached
    }
    SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;    // Set SVS/SVM low side to new level
    while((PMMIFG & SVSMLDLYIFG) == 0);                                 // Wait till SVM is settled
    PMMCTL0_H = 0x00;                                                   // Lock PMM registers for write access
}
*/

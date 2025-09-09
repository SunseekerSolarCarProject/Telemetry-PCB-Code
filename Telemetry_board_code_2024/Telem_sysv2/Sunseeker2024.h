/*
 * Sunseeker Telemetry 2023
 *
 * Last modified may 2021 by B. Bazuin
 * Last modified March 2024 by D. Thompson
 *
 * Main CLK     :  MCLK  = XT2     = 20 MHz
 * Sub-Main CLK :  SMCLK = XT2/2   = 10  MHz
 * Aux CLK      :  ACLK  = XT1     = 32.768 kHz
 *
 */

#ifndef SUNSEEKER2024_H_
#define SUNSEEKER2024_H_

#include <msp430fr2476.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <float.h>

//#include "UART.h"
//#include "CAN.h"
//#include "Modem_RS232.h"
//#include "can_FIFO.h"
//#include "decode_packet.h"
//#include "rtcic_i2c.h"
//#include "RTC.h"

void clock_init(void);
void timerB_init(void);
void timer_b0(void);
void io_init(void);
void rtc_init(void);
int insert_time(char *time_string);
void getRTCTime(int *h, int *m, int *s);
void RTC_ISR(void);
void USCI_A0_ISR(void);
void WDT_ISR(void);

void packet_init(void);
void decode();

int lookup(unsigned int address, int *off, int *pos, int *pck, int *row);

static inline void delay(void)
{
  volatile int jj;
  volatile int ii;
  // Using count-down since it uses one less instruction to compare (not really necessary here, but CCS recommends it anyway): Ultra-Low Power Advisor > Rule 13.1 (https://software-dl.ti.com/ccs/esd/documents/dmed/HTML/MSP430/1544.html)
  for (ii = 4; ii > 0; ii--)
  {
    for (jj = 1000; jj > 0; jj--)
    {
      asm(" nop"); //The space is necessary or else the assembler thinks "nop" is a label!
    }
  }
}

// Event timing
#define MCLK_RATE       32768*487 = 15958016// Hz
#define SMCLK_RATE      3989504    // Hz
#define ACLK_RATE       32768   // Hz
#define TICK_RATE       100      // Hz
#define STATUS_COUNT     200           // Number of ticks per event: 0.1 sec
#define CAN_COMMS_COUNT     10            // Number of ticks per event:  1 sec
#define HS_COMMS_SPEED      100*5           // Number of ticks per event: 1 sec
#define LS_COMMS_SPEED      100*30           // Number of ticks per event: 30 sec
#define ST_COMMS_SPEED      100*60           // Number of ticks per event: 60 sec

// Constant Definitions
#define TRUE                1
#define FALSE               0

#ifndef MODEM_BR1
#define MODEM_BR1 9600
#define MODEM_UCBRS1 0x04 // 2*ROUND(SMCLK_RATE/MODEM_BR-INT(SMCLK_RATE/MODEM_BR))*8
#endif

#ifndef MODEM_BR2
#define MODEM_BR2 115200
#define MODEM_UCBRS2 0x0A // 2*ROUND(SMCLK_RATE/MODEM_BR-INT(SMCLK_RATE/MODEM_BR))*8
#endif

#ifndef USB_BR
#define USB_BR 9600
#define USB_UCBRS 0x04 // 2*ROUND(SMCLK_RATE/UART_BR-INT(SMCLK_RATE/UART_BR))*8
#endif

#ifndef UART_BR
#define UART_BR 19200
#define UART_UCBRS 0x0D // 2*ROUND(SMCLK_RATE/UART_BR-INT(SMCLK_RATE/UART_BR))*8
#endif

/******************** Pin Definitions *************************/

// PORT 1
#define LED             0x01
#define CAN_SCLK        0x02
#define CAN_SIMO        0x04
#define CAN_SOMI        0x08
#define RS232_TX        0x10
#define RS232_RX        0x20
#define P16             0x40
#define P17             0x80
#define P1_UNUSED       0x40 | 0x80

// PORT 2
#define XT1OUT          0x01
#define XT1IN           0x02
#define P22             0x04
#define P23             0x08
#define LEDR            0x10
#define LEDG            0x20
#define LEDY0           0x40
#define LEDY1           0x80
#define P2_UNUSED       0x04 | 0x08

// PORT 3
#define CAN_RSTn        0x01
#define CAN_RXB0        0x02
#define CAN_RXB1        0x04
#define CAN_INTn        0x08
#define P34             0x10
#define P35             0x20
#define P36             0x40
#define CAN_CSn         0x80
#define P3_UNUSED       0x10 | 0x20 | 0x40

// PORT 4
#define P40             0x01
#define P41             0x02
#define P42             0x04
#define P43             0x08
#define P44             0x10
#define P45             0x20
#define P46             0x40
#define P47             0x80
#define P4_UNUSED       0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40 | 0x80

// PORT 5
#define P50             0x01
#define P51             0x02
#define P52             0x04
#define P53             0x08
#define P54             0x10
#define P55             0x20
#define P56             0x40
#define P57             0x80
#define P5_UNUSED       0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40 | 0x80

// PORT 6
#define P60             0x01
#define P61             0x02
#define P62             0x04
#define P63             0x08
#define P64             0x10
#define P65             0x20
#define P66             0x40
#define P67             0x80
#define P6_UNUSED       0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40 | 0x80

// Macros for leds on the board
#define LEDTOG          P1OUT ^= LED
#define LEDGTOG         P2OUT ^= LEDG
#define LEDRTOG         P2OUT ^= LEDR
#define LEDY1TOG        P2OUT ^= LEDY0
#define LEDY2TOG        P2OUT ^= LEDY1
#define LEDOFF          P1OUT = ~LED
#define LEDGOFF         P2OUT = ~LEDG
#define LEDROFF         P2OUT = ~LEDR
#define LEDY1OFF        P2OUT = ~LEDY0
#define LEDY2OFF        P2OUT = ~LEDY1
#define LEDON           P1OUT = LED
#define LEDGON          P2OUT = LEDG
#define LEDRON          P2OUT = LEDR
#define LEDY1ON         P2OUT = LEDY0
#define LEDY2ON         P2OUT = LEDY1


// Transmit Packet Info
#define HF_MSG_PACKET  9    //number of messages per packet in high frequency
#define LF_MSG_PACKET  0   //number of messages per packet in low frequency
#define ST_MSG_PACKET  0    //number of messages that we receive and don't send out
#define No_MSG_PACKET  25    //number of messages that we receive and don't send out
#define LOOKUP_ROWS HF_MSG_PACKET+LF_MSG_PACKET+ST_MSG_PACKET+No_MSG_PACKET
#define NAME_LOOKUP_ROWS LOOKUP_ROWS
//#define TIME_SIZE 30        //number of characters in time
#define MSG_SIZE  30        //number of characters in single message

// MPPT Controller Addresses
#define MPPT_CAN_BASE       0x600       // CAN Base Address to send RTR requests
#define MPPT_CAN_ONOFF      0x10        // CAN Base Address to send on/off messages to the MPPTs
#define MPPT_CAN_ADDRESS1       0x00        // Address to specify MPPT 1
#define MPPT_CAN_ADDRESS2       0x01        // Address to specify MPPT 2

// Motor controller 1 CAN base address and packet offsets
#define MC1_CAN_BASE        0x400       // High = CAN1_SERIAL Number        Low = 0x00004003                    P=1s
#define MC1_LIMITS           0x01        // High = CAN_Err,Active Motor      Low = Error & Limit flags           P=200ms
#define MC1_BUS              0x02        // High = Bus Current               Low = Bus Voltage                   P=200ms
#define MC1_VELOCITY         0x03        // High = Velocity (m/s)            Low = Velocity (rpm)                P=200ms
#define MC1_PHASE            0x04        // High = Phase C Current           Low = Phase B Current               P=200ms
#define MC1_V_VECTOR         0x05        // High = Vd vector                 Low = Vq vector                     P=200ms
#define MC1_I_VECTOR         0x06        // High = Id vector                 Low = Iq vector                     P=200ms
#define MC1_BEMF_VECTOR      0x07        // High = BEMFd vector              Low = BEMFq vector                  P=200ms
#define MC1_RAIL1            0x08        // High = 15V                       Low = Reserved                      P=1s
#define MC1_RAIL2            0x09        // High = 3.3V                      Low = 1.9V                          P=1s
//#define MC_FAN            0x0A        // High = Reserved                  Low = Reserved                      P=
#define MC1_TEMP1            0x0B        // High = Heatsink Temp (case)      Low = Motor Temp (internal)         P=1s
#define MC1_TEMP2            0x0C        // High = Reserved                  Low = DSP Temp                      P=1s
//#define MC_TEMP3          0x0D        // High = Outlet Temp               Low = Capacitor Temp                P=
#define MC1_CUMULATIVE       0x0E        // High = DC Bus AmpHours (A-Hr)    Low = Odometer  (m)                 P=1s
#define MC1_SLIPSPEED        0x17        // High = Slip Speed (Hz       )    Low = Reserved                      P=200ms

// Motor controller 2 CAN base address and packet offsets
#define MC2_CAN_BASE         0x420       // High = CAN1_SERIAL Number        Low = 0x00004003                    P=1s
#define MC2_LIMITS           0x01        // High = CAN_Err,Active Motor      Low = Error & Limit flags           P=200ms
#define MC2_BUS              0x02        // High = Bus Current               Low = Bus Voltage                   P=200ms
#define MC2_VELOCITY         0x03        // High = Velocity (m/s)            Low = Velocity (rpm)                P=200ms
#define MC2_PHASE            0x04        // High = Phase C Current           Low = Phase B Current               P=200ms
#define MC2_V_VECTOR         0x05        // High = Vd vector                 Low = Vq vector                     P=200ms
#define MC2_I_VECTOR         0x06        // High = Id vector                 Low = Iq vector                     P=200ms
#define MC2_BEMF_VECTOR      0x07        // High = BEMFd vector              Low = BEMFq vector                  P=200ms
#define MC2_RAIL1            0x08        // High = 15V                       Low = Reserved                      P=1s
#define MC2_RAIL2            0x09        // High = 3.3V                      Low = 1.9V                          P=1s
//#define MC_FAN            0x0A        // High = Reserved                  Low = Reserved                      P=
#define MC2_TEMP1            0x0B        // High = Heatsink Temp (case)      Low = Motor Temp (internal)         P=1s
#define MC2_TEMP2            0x0C        // High = Reserved                  Low = DSP Temp                      P=1s
//#define MC_TEMP3          0x0D        // High = Outlet Temp               Low = Capacitor Temp                P=
#define MC2_CUMULATIVE       0x0E        // High = DC Bus AmpHours (A-Hr)    Low = Odometer  (m)                 P=1s
#define MC2_SLIPSPEED        0x17        // High = Slip Speed (Hz       )    Low = Reserved                      P=200ms


// Driver controls CAN base address and packet offsets
#define DC_CAN_BASE         0x500       // High = CAN1_SERIAL Number        Low = "TRIb" string                 P=1s
#define DC_DRIVE            0x01        // High = Motor Current Setpoint    Low = Motor Velocity Setpoint       P=100ms
#define DC_POWER            0x02        // High = Bus Current Setpoint      Low = Unused                        P=100ms
#define DC_RESET            0x03        // High = Unused                    Low = Unused                        P=
#define DC_SWITCH           0x04        // High = Switch position           Low = Switch state change           P=100ms

// Steering Wheel CAN base address and packet offsets
#define STW_CAN_BASE         0x540       // High = CAN1_SERIAL Number        Low = "TRIb" string                 P=1s
#define STW_SWITCH           0x01        // High = Switch position           Low = Switch state change           P=100ms
#define STW_HORN       0x0001
#define STW_IND_L      0x0002
#define STW_IND_R      0x0004
#define STW_REGEN      0x0008
#define STW_CRUISE     0x0010

//Battery Protection System base address and packet offsets
#define BP_CAN_BASE         0x580       // High = "BPV1" string or nulls    Low = CAN1_SERIAL Number            P=10s
#define BP_VMAX             0x01        // High = Max. Voltage Value        Low = Max. Voltage Cell Num.        P=10s
#define BP_VMIN             0x02        // High = Min. Voltage Value        Low = Min. Voltage Cell Num.        P=10s
#define BP_TMAX             0x03        // High = Max. Temperature          Low = Max. Temperature Cell         P=10s
#define BP_PCDONE           0x04        // High = "BPV2" or "0000" string   Low = CAN1_SERIAL Number            P=When Ready
#define BP_ISH              0x05        // High = Shunt Current             Low = Battery Voltage               P=1s
#define BP_RESET            0x07        // High = Unused                    Low = Unused                        P=

//Battery Protection System base address and packet offsets
#define AC_CAN_BASE         0x5C0       // High = "ACV1" string or nulls    Low = CAN1_SERIAL Number            P=10s
#define AC_M1               0x01        // High = Array Voltage Average     Low = Array Current Average         P=10s
#define AC_M2               0x02        // High = Array Voltage Average     Low = Array Current Average         P=10s
#define AC_M3               0x03        // High = Array Voltage Average     Low = Array Current Average         P=10s
#define AC_ISH              0x04        // High = Shunt Current             Low = Battery Voltage               P=1s
#define AC_TMAX             0x05        // High = Max. Temperature          Low = Max. Temperature MPPT         P=10s
#define AC_TVAL1            0x06        // High = Temp AC1                  Low = Temp AC2                      P=10s
#define AC_TVAL2            0x07        // High = Temp AC3                  Low = Reserved                      P=10s
#define AC_BP_CHARGE        0x08        // High = "ACV1" or "0000" string   Low = CAN1_SERIAL Number            P=When Charge
//#define   AC_ISUM     0x09        // High = Shunt Current mA          Low = current sum mA/sec

//Telemetry base address and packet offsets
#define TM_CAN_BASE         0x5E0       // High =     Low = CAN_SERIAL Number            P=10s

static int addr_lookup[LOOKUP_ROWS][5] = {
  //address                           ASCII Offset      MSG_REC position        Packet(0-HF:1-LF:2-Status)      Filter Priority
  {MC1_CAN_BASE,                       0,                0x0001,                   3,                            29},                    //0-0x400       High = CAN1_SERIAL Number        Low = "TRIa" string
  {MC1_CAN_BASE + MC1_LIMITS,          8,                0x0100,                   0,                            15},                    //1-0x01        High = Active Motor              Low = Error & Limit flags
  {MC1_CAN_BASE + MC1_BUS,             0,                0x0001,                   0,                             1},                    //2-0x02        High = Bus Current               Low = Bus Voltage
  {MC1_CAN_BASE + MC1_VELOCITY,        1,                0x0002,                   0,                             2},                    //3-0x03        High = Velocity (m/s)            Low = Velocity (rpm)
  {MC1_CAN_BASE + MC1_PHASE,           1,                0x0001,                   3,                            30},                    //4-0x04        High = Phase A Current           Low = Phase B Current
  {MC1_CAN_BASE + MC1_V_VECTOR,        2,                0x0002,                   3,                            31},                    //5-0x05        High = Vd vector                 Low = Vq vector
  {MC1_CAN_BASE + MC1_I_VECTOR,        3,                0x0004,                   3,                            32},                    //6-0x06        High = Id vector                 Low = Iq vector
  {MC1_CAN_BASE + MC1_BEMF_VECTOR,     4,                0x0008,                   3,                            33},                    //7-0x07        High = BEMFd vector              Low = BEMFq vector
  {MC1_CAN_BASE + MC1_RAIL1,           1,                0x0002,                   3,                            16},                    //8-0x08        High = 15V                       Low = Reserved
  {MC1_CAN_BASE + MC1_RAIL2,           2,                0x0004,                   3,                            17},                    //9-0x09        High = 3.3V                      Low = 1.9V
  {MC1_CAN_BASE + MC1_TEMP1,           0,                0x0001,                   3,                             7},                    //10-0x0B       High = Heatsink Temp             Low = Motor Temp
  {MC1_CAN_BASE + MC1_TEMP2,           1,                0x0002,                   3,                             8},                    //11-0x0C       High = Inlet Temp                Low = CPU Temp
  {MC1_CAN_BASE + MC1_CUMULATIVE,      3,                0x0008,                   3,                            18},                    //12-x0E        High = DC Bus AmpHours           Low = Odometer
  {MC1_CAN_BASE + MC1_SLIPSPEED,       5,                0x0010,                   3,                            34},                    //13-0x17       High =Slip Speed (Hz)            Low = Reserved
  {MC2_CAN_BASE,                       0,                0x0001,                   3,                            29},                    //0-0x400       High = CAN1_SERIAL Number        Low = "TRIa" string
  {MC2_CAN_BASE + MC2_LIMITS,          8,                0x0100,                   0,                            15},                    //1-0x01        High = Active Motor              Low = Error & Limit flags
  {MC2_CAN_BASE + MC2_BUS,             0,                0x0001,                   0,                             1},                    //2-0x02        High = Bus Current               Low = Bus Voltage
  {MC2_CAN_BASE + MC2_VELOCITY,        1,                0x0002,                   0,                             2},                    //3-0x03        High = Velocity (m/s)            Low = Velocity (rpm)
  {MC2_CAN_BASE + MC2_PHASE,           1,                0x0001,                   3,                            30},                    //4-0x04        High = Phase A Current           Low = Phase B Current
  {MC2_CAN_BASE + MC2_V_VECTOR,        2,                0x0002,                   3,                            31},                    //5-0x05        High = Vd vector                 Low = Vq vector
  {MC2_CAN_BASE + MC2_I_VECTOR,        3,                0x0004,                   3,                            32},                    //6-0x06        High = Id vector                 Low = Iq vector
  {MC2_CAN_BASE + MC2_BEMF_VECTOR,     4,                0x0008,                   3,                            33},                    //7-0x07        High = BEMFd vector              Low = BEMFq vector
  {MC2_CAN_BASE + MC2_RAIL1,           1,                0x0002,                   3,                            16},                    //8-0x08        High = 15V                       Low = Reserved
  {MC2_CAN_BASE + MC2_RAIL2,           2,                0x0004,                   3,                            17},                    //9-0x09        High = 3.3V                      Low = 1.9V
  {MC2_CAN_BASE + MC2_TEMP1,           0,                0x0001,                   3,                             7},                    //10-0x0B       High = Heatsink Temp             Low = Motor Temp
  {MC2_CAN_BASE + MC2_TEMP2,           1,                0x0002,                   3,                             8},                    //11-0x0C       High = Inlet Temp                Low = CPU Temp
  {MC2_CAN_BASE + MC2_CUMULATIVE,      3,                0x0008,                   3,                            18},                    //12-x0E        High = DC Bus AmpHours           Low = Odometer
  {MC2_CAN_BASE + MC2_SLIPSPEED,       5,                0x0010,                   3,                            34},                    //13-0x17       High =Slip Speed (Hz)            Low = Reserved
  {DC_CAN_BASE,                        12,               0x0800,                   3,                            41},                    //28-0x500      High = CAN1_SERIAL Number        Low = "TRIb" string
  {DC_CAN_BASE + DC_DRIVE,             2,                0x0004,                   0,                            23},                    //29-0x01       High = Motor Current Setpoint    Low = Motor Velocity Setpoint
  {DC_CAN_BASE + DC_POWER,             9,                0x0200,                   3,                            24},                    //30-0x02       High = Bus Current Setpoint      Low = Unused
  {DC_CAN_BASE + DC_RESET,             13,               0x1000,                   3,                            42},                    //31-0x03       High = Unused                    Low = Unused
  {DC_CAN_BASE + DC_SWITCH,            3,                0x0008,                   0,                            25},                    //32-0x04       High = Switch position           Low = Switch state change
  {BP_CAN_BASE,                        14,                0x2000,                   3,                            43},                   //33-0x580      High = BPV2" string or nulls     Low = CAN1_SERIAL Number
  {BP_CAN_BASE + BP_VMAX,              4,                0x0010,                   0,                            11},                    //34-0x01       High = Max Voltage               Low = Cell Number
  {BP_CAN_BASE + BP_VMIN,              5,                0x0020,                   0,                            12},                    //35-0x02       High = Min Voltage               Low = Cell Number
  {BP_CAN_BASE + BP_TMAX,              6,                0x0040,                   0,                            13},                    //36-0x03       High = Temp Max                  Low = Cell Number
  {BP_CAN_BASE + BP_PCDONE,            15,                0x0200,                   3,                            44},                   //37-0x04       High = "BPV1" string             Low = CAN1_SERIAL Number
  {BP_CAN_BASE + BP_ISH,               7,                0x0080,                   0,                             5},                    //38-0x05       High = Shunt Current             Low = Battery Voltage
  {AC_CAN_BASE,                        16,                0x8000,                   3,                            45},                   //39-0x5C0      High = ACV1" string or nulls     Low = CAN1_SERIAL Number
  {AC_CAN_BASE + AC_M1,                11,                0x0800,                   3,                            26},                   //40-0x01       High = Array Voltage Average     Low = Array Current Average
  {AC_CAN_BASE + AC_M2,                12,                0x1000,                   3,                            27},                   //41-0x02       High = Array Voltage Average     Low = Array Current Average
  {AC_CAN_BASE + AC_M3,                13,                0x2000,                   3,                            28},                   //42-0x03       High = Array Voltage Average     Low = Array Current Average
  {AC_CAN_BASE + AC_ISH,               5,                0x0020,                   3,                             6},                    //43-0x04       High = Shunt Current             Low = Battery Voltage
  {AC_CAN_BASE + AC_TMAX,              7,                0x0080,                   3,                            14},                    //44-0x05       High = Max. Temperature          Low = Max. Temperature MPPT
  {AC_CAN_BASE + AC_TVAL1,             17,                0x1000,                   3,                            46},                   //45-0x03       High = Temp AC1                  Low = Temp AC2
  {AC_CAN_BASE + AC_TVAL2,             18,                0x2000,                   3,                            47},                   //46-0x04       High = Temp AC3                  Low = Reserved
  {AC_CAN_BASE + AC_BP_CHARGE,         19,                0x4000,                   3,                            48},                   //47-0x05       High = "ACV1" or "0000" string   Low = CAN1_SERIAL Number
};
// removed
//{MC_CAN_BASE1 + MC_FAN,             -,                0x----,                   -,                            --},                    //xx-0x0A       High = Fan speed (rpm)           Low = Fan drive (%)
//{MC_CAN_BASE1 + MC_TEMP3,           -,                0x----,                   -,                            --},                    //xx-0x0D       High = Outlet Temp               Low = Capacitor Temp
//{MC_CAN_BASE2 + MC_FAN,             -,                0x----,                   -,                            --},                    //xx-0x0A       High = Fan speed (rpm)           Low = Fan drive (%)
//{MC_CAN_BASE2 + MC_TEMP3,           -,                0x----,                   -,                            --},                    //xx-0x0D       High = Outlet Temp               Low = Capacitor Temp


//static char lut_blacklist[] = {32,11,4,5,11,12,13,14,15,23,24,25,3,26,27,28};
//static char lut_blacklist[] = {44};
static char lut_blacklist[] = {49};

static char *name_lookup[NAME_LOOKUP_ROWS] = {
  //name
  "MC1BAS", //MC1_CAN_BASE
  "MC1LIM", //MC1_CAN_BASE + MC1_LIMITS
  "MC1BUS", //MC1_CAN_BASE + MC1_BUS
  "MC1VEL", //MC1_CAN_BASE + MC1_VELOCITY
  "MC1PHA", //MC1_CAN_BASE + MC1_PHASE
  "MC1VVC", //MC1_CAN_BASE + MC1_V_VECTOR
  "MC1IVC", //MC1_CAN_BASE + MC1_I_VECTOR
  "MC1BEM", //MC1_CAN_BASE + MC1_BEMF_VECTOR
  "MC1RL1", //MC1_CAN_BASE + MC1_RAIL1
  "MC1RL2", //MC1_CAN_BASE + MC1_RAIL2
  "MC1TP1", //MC1_CAN_BASE + MC1_TEMP1
  "MC1TP2", //MC1_CAN_BASE + MC1_TEMP2
  "MC1CUM", //MC1_CAN_BASE + MC1_CUMULATIVE
  "MC1SLS", //MC1_CAN_BASE + MC1_SLIPSPEED
  "MC2BAS", //MC2_CAN_BASE
  "MC2LIM", //MC2_CAN_BASE + MC2_LIMITS
  "MC2BUS", //MC2_CAN_BASE + MC2_BUS
  "MC2VEL", //MC2_CAN_BASE + MC2_VELOCITY
  "MC2PHA", //MC2_CAN_BASE + MC2_PHASE
  "MC2VVC", //MC2_CAN_BASE + MC2_V_VECTOR
  "MC2IVC", //MC2_CAN_BASE + MC2_I_VECTOR
  "MC2BEM", //MC2_CAN_BASE + MC2_BEMF_VECTOR
  "MC2RL1", //MC2_CAN_BASE + MC2_RAIL1
  "MC2RL2", //MC2_CAN_BASE + MC2_RAIL2
  "MC2TP1", //MC2_CAN_BASE + MC2_TEMP1
  "MC2TP2", //MC2_CAN_BASE + MC2_TEMP2
  "MC2CUM", //MC2_CAN_BASE + MC2_CUMULATIVE
  "MC2SLS", //MC2_CAN_BASE + MC2_SLIPSPEED
  "DC_BAS", //DC_CAN_BASE
  "DC_DRV", //DC_CAN_BASE + DC_DRIVE
  "DC_POW", //DC_CAN_BASE + DC_POWER
  "DC_RST", //DC_CAN_BASE + DC_RESET
  "DC_SWC", //DC_CAN_BASE + DC_SWITCH
  "BP_BAS", //BP_CAN_BASE
  "BP_VMX", //BP_CAN_BASE + BP_VMAX
  "BP_VMN", //BP_CAN_BASE + BP_VMIN
  "BP_TMX", //BP_CAN_BASE + BP_TMAX
  "BP_PCD", //BP_CAN_BASE + BP_PCDONE
  "BP_ISH", //BP_CAN_BASE + BP_ISH
  "AC_BAS", //AC_CAN_BASE
  "AC_MP1", //AC_CAN_BASE + AC_M1
  "AC_MP2", //AC_CAN_BASE + AC_M2
  "AC_MP3", //AC_CAN_BASE + AC_M3
  "AC_ISH", //AC_CAN_BASE + AC_ISH
  "AC_TMX", //AC_CAN_BASE + AC_TMAX
  "AC_TV1", //AC_CAN_BASE + AC_TVAL1
  "AC_TV2", //AC_CAN_BASE + AC_TVAL2
  "AC_BPC", //AC_CAN_BASE + AC_BP_CHARGE
};

// removed
// "MC1FAN", //MC_CAN_BASE1 + MC_FAN
// "MC1TP3", //MC_CAN_BASE1 + MC_TEMP3
// "MC2FAN", //MC_CAN_BASE2 + MC_FAN
// "MC2TP3", //MC_CAN_BASE2 + MC_TEMP3

#endif /* SUNSEEKER2024_H_ */

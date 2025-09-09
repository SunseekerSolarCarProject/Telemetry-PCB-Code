//
//
//

// Include files
#include <msp430fr2476.h>
#include "Sunseeker2024.h"


void rtc_init(void)
{
    // RTC count re-load compare value at 32.
    // 1024/32768 * 32 = 1 sec.
    RTCMOD = 32-1;
                                            // Initialize RTC
    // Source = ACLK = REFO, divided by 1024
    RTCCTL = RTCSS__XT1CLK | RTCSR | RTCPS__1024 | RTCIE;
//    RTCCTL |= RTCIE;
}

/*************************************************************
/ Name: getRTCTime
/ IN: int *hour, int *minute, int *second
/ OUT:  void
/ DESC:  This function returns the current time of the Real time clock
************************************************************/
void getRTCTime(int *h, int *m, int *s)
{
  extern char rtchrs, rtcmin, rtcsec; //MCP7940M format

  *h = (rtchrs / 10 * 16 +  rtchrs % 10);
  *m = (rtcmin / 10 * 16 +  rtcmin % 10);
  *s = (rtcsec / 10 * 16 +  rtcsec % 10);
}

//
//   char time_msg[17];// = "TL_TIM,HH:MM:SS\r\n";
//
int insert_time(char *time_string)
{
  char h1, h0, m1, m0, s1, s0;
  extern int thrs, tmin, tsec;

  h1 = ((thrs>>4) & 0x0F)+'0';
  h0 = (thrs & 0x0F)+'0';
  time_string[7] = h1;
  time_string[8] = h0;

  m1 = ((tmin>>4) & 0x0F)+'0';
  m0 = (tmin & 0x0F)+'0';
  time_string[10] = m1;
  time_string[11] = m0;

  s1 = ((tsec>>4) & 0x0F)+'0';
  s0 = (tsec & 0x0F)+'0';
  time_string[13] = s1;
  time_string[14] = s0;

  return 1;
}


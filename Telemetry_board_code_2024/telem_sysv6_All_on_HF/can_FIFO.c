//
// Telemetry Messgae FIFO
//
#include "Sunseeker2024.h"
#include "can.h"
#include "can_FIFO.h"

void can_fifo_INIT(void)
{
	can_struct *temptestPt;
  
  temptestPt = &can_queue.msg_fifo[0];
  can_queue.PutPt = temptestPt;
  can_queue.GetPt = temptestPt;
} 

int can_fifo_PUT(can_message_fifo *queue, can_struct toPut)
{
    extern volatile unsigned char can_fifo_full;

    can_struct *next = queue->PutPt + 1;
    if (next == &queue->msg_fifo[msg_fifo_size]) {
        next = &queue->msg_fifo[0];  // wrap
    }

    // Full? (next would collide with GetPt)
    if (next == queue->GetPt) {
        can_fifo_full = TRUE;
        return 0; // failure, FIFO full (and we did NOT overwrite)
    }

    // Safe to write
    *(queue->PutPt) = toPut;
    queue->PutPt = next;
    can_fifo_full = FALSE;
    return 1;
}

int can_fifo_GET(can_message_fifo *queue, can_struct *toGet)
{
	extern volatile unsigned char can_fifo_full;

  if(queue->PutPt == queue->GetPt)
  {
	  can_fifo_full = FALSE;
	  return(0);	//failure FIFO empty
  }
  else
  {
    *toGet = *(queue->GetPt++);
    if(queue->GetPt == &queue->msg_fifo[msg_fifo_size])
    {
      queue->GetPt = &queue->msg_fifo[0];	//need to wrap around
    }
    can_fifo_full = FALSE;
    return(1);
  }
}

int can_fifo_STAT(can_message_fifo *queue)
{
  return (queue->GetPt != queue->PutPt);
}


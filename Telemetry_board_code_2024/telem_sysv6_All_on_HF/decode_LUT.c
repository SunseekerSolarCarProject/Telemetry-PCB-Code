//
// Decode and Lookup Table Routines
//

#include "Sunseeker2024.h"
#include "can.h"
#include "can_FIFO.h"
#include "decode_packet.h"

extern hf_packet pckHF;
extern lf_packet pckLF;
extern status_packet pckST;

extern unsigned int can_mask0, can_mask1; //Mask 0 should always be the lower value(higher priority)

static const char init_pre_msg[8] = "ABCDEF\r\n";
static const char init_msg[30] = "XXXXXX,0xHHHHHHHH,0xHHHHHHHH\r\n";
static const char init_time_msg[17] = "TL_TIM,HH:MM:SS\r\n";
static const char init_post_msg[9] = "UVWXYZ\r\n\0";

//static int next_HF_index = -1;
//static int next_LF_index = -1;

#define priority(row) addr_lookup[row][4]
#define address(row) addr_lookup[row][0]

unsigned int lookup_next(int pri);

/*void decode()
{
  int offset;
  int position;
  int pck;
  int row;
  int i;
  char a0, a1, dflag;
  static can_struct current;
  static pck_message *xmit_string;
  
  dflag = 0x00;
  
  if(can_fifo_GET(&can_queue, &current))
  {
    if(lookup(current.address, &offset, &position, &pck, &row))
    {
      //if(row == can_mask1) can_mask1 = lookup_next(priority(row)); 
      //else if(row == can_mask0) {
      //	can_mask0 = can_mask1;
      //	can_mask1 = lookup_next(priority(can_mask0));
      //} else {
      	//shouldn't happen!
      //}
      //can1_sources(address(can_mask0), address(can_mask1));    	
   
      if(pck == 0)
      {
        if( (pckHF.msg_filled & position) == 0)
        {
          xmit_string = &(pckHF.xmit[offset]);
          pckHF.msg_filled |= position;
          dflag = 0xFF;
        }
      }
      else if(pck == 1)
      {  
        if( (pckLF.msg_filled & position) == 0)
        {
          xmit_string = &(pckLF.xmit[offset]);
          pckLF.msg_filled |= position;
          dflag = 0xFF;
        }
      }
      else if(pck == 2)
      {  
        if( (pckST.msg_filled & position) == 0)
        {
          xmit_string = &(pckST.xmit[offset]);
          pckST.msg_filled |= position;
          dflag = 0xFF;
        }
      }
      else
      {
        dflag = 0x00;
      }
      
      if (dflag == 0xFF)
      {
        for(i=0;i<4;i++)
        {
          a1 = ((current.data.data_u8[i]>>4) & 0x0F)+'0';
          if (a1 > '9')
            a1 = a1 - '0' - 0x0a + 'A';
          a0 = (current.data.data_u8[i] & 0x0F)+'0';
          if (a0 > '9')
            a0 = a0 - '0' - 0x0a + 'A';
          xmit_string->message[2*i+9]=a1;
          xmit_string->message[2*i+10]=a0;
        }
        for(i=0;i<4;i++)
        {
          a1 = ((current.data.data_u8[i+4]>>4) & 0x0F)+'0';
          if (a1 > '9')
            a1 = a1 - '0' - 0x0a + 'A';
          a0 = (current.data.data_u8[i+4] & 0x0F)+'0';
          if (a0 > '9')
            a0 = a0 - '0' - 0x0a + 'A';
          xmit_string->message[2*i+20]=a1;
          xmit_string->message[2*i+21]=a0;
        }
      }
    }
  }
}*/
/*
void decode()
{
    can_struct scratch;
    // Temporary snapshot: gather all messages currently in the hardware FIFO
    can_struct heap[ msg_fifo_size ];
    int nready = 0;
    while(can_fifo_STAT(&can_queue) && nready < msg_fifo_size) {
        // pull each one out of the queue into "heap"
        if(can_fifo_GET(&can_queue, &scratch))
            heap[nready++] = scratch;
    }
    // Now 'heap[0..nready_1]' contains all waiting CAN messages.
    // We’ll re_enqueue those we do NOT process right away, but process exactly 1 message
    // per decode() call (so that HS/LS interrupts can fire in between, etc.)

    // 1) Among 'heap', find the row with smallest priority (for its packet_type).
    int chosen_index = -1;
    int chosen_row   = -1;
    int chosen_pri   = 0x7FFF;
    int i;
    for(i=0; i<nready; i++)
    {
        int row, off, pos, pck;
        if(lookup(heap[i].address, &off, &pos, &pck, &row))
        {
            int pri = addr_lookup[row][4];
            if(pri < chosen_pri)
            {
                chosen_pri   = pri;
                chosen_row   = row;
                chosen_index = i;
            }
        }
    }
    // 2) If we found a “chosen_row”, then fill it
    if(chosen_index >= 0)
    {
        int row = chosen_row;
        int off = addr_lookup[row][1];
        int pos = addr_lookup[row][2];
        int pck = addr_lookup[row][3]; // 0=HF,1=LF,2=ST

        pck_message *xmit_ptr = NULL;
        if(pck==0)
        {
            // If that HF_slot is not yet filled:
            if((pckHF.msg_filled & pos)==0)
            {
                xmit_ptr = &pckHF.xmit[ off ];
                pckHF.msg_filled |= pos;
            }
        }
        else if(pck==1)
        {
            if((pckLF.msg_filled & pos)==0)
            {
                xmit_ptr = &pckLF.xmit[ off ];
                pckLF.msg_filled |= pos;
            }
        }
        else if(pck==2)
        {
            // status (you currently have ST_MSG_PACKET=0, so probably never used)
            if((pckST.msg_filled & pos)==0)
            {
                xmit_ptr = &pckST.xmit[ off ];
                pckST.msg_filled |= pos;
            }
        }
        // Now *hex*_dump the 8 data bytes from heap[chosen_index].data
        if(xmit_ptr != NULL)
        {
            unsigned char *db = heap[chosen_index].data.data_u8;
            int b;
            for(b=0; b<8; b++)
            {
                unsigned char nyb1 = ((db[b]>>4)&0x0F)+'0';
                if(nyb1 > '9') nyb1 = nyb1 - '0' - 0x0A + 'A';
                unsigned char nyb0 = (db[b]&0x0F)+'0';
                if(nyb0 > '9') nyb0 = nyb0 - '0' - 0x0A + 'A';

                xmit_ptr->message[2*b + 9]  = nyb1;
                xmit_ptr->message[2*b +10]  = nyb0;
            }
        }
    }

    // 3) Re_enqueue any “unprocessed” heap[] entries back into the can_queue
    for(i=0; i<nready; i++)
    {
        if(i != chosen_index)
            can_fifo_PUT(&can_queue, heap[i]);
    }
}
*/

void decode(void)
{
    can_struct scratch;
    can_struct heap[msg_fifo_size];
    int nready = 0;
    int chosen_index = -1;
    uint64_t chosen_row = 0xFFFFFFFFu;
    int chosen_pri = 0x7FFF;
    int i;          /* C89: declare at top */
    int k;          /* C89: for inner byte loops */

    /* drain queue into a local heap */
    while (can_fifo_STAT(&can_queue) && nready < msg_fifo_size) {
        if (can_fifo_GET(&can_queue, &scratch))
            heap[nready++] = scratch;
    }

    /* pick one by priority */
    for (i = 0; i < nready; i++) {
        uint64_t row, off, pck;   /* OK to declare here (still before any code inside this block) */
        uint64_t pos;
        if (lookup(heap[i].address, &off, &pos, &pck, &row)) {
            int pri = (int)addr_lookup[row][4];
            if (pri < chosen_pri) {
                chosen_pri   = pri;
                chosen_row   = row;
                chosen_index = i;
            }
        }
    }

    if (chosen_index >= 0) {
        uint64_t row = (uint64_t)chosen_row;
        uint64_t off = (uint64_t)addr_lookup[row][1];
        uint64_t pos = (uint64_t)addr_lookup[row][2];
        uint64_t pck = (uint64_t)addr_lookup[row][3];
        pck_message *x = NULL;

        if (pck == 0) {
            if ((pckHF.msg_filled & pos) == 0) {
                x = &pckHF.xmit[off];
                pckHF.msg_filled |= pos;
            }
        } else if (pck == 1) {
            if ((pckLF.msg_filled & pos) == 0) {
                x = &pckLF.xmit[off];
                pckLF.msg_filled |= pos;
            }
        } else if (pck == 2) {
            if ((pckST.msg_filled & pos) == 0) {
                x = &pckST.xmit[off];
                pckST.msg_filled |= pos;
            }
        }

        if (x != NULL) {
            unsigned char *db = heap[chosen_index].data.data_u8;

            /* first 4 bytes -> positions 9..16 */
            for (k = 0; k < 4; k++) {
                unsigned char a1 = ((db[k] >> 4) & 0x0F) + '0';
                if (a1 > '9') a1 = a1 - '0' - 0x0A + 'A';
                unsigned char a0 = (db[k] & 0x0F) + '0';
                if (a0 > '9') a0 = a0 - '0' - 0x0A + 'A';
                x->message[2*k + 9]  = a1;
                x->message[2*k + 10] = a0;
            }
            /* last 4 bytes -> positions 20..27 (skip ",0x" at 18..19) */
            for (k = 0; k < 4; k++) {
                unsigned char a1 = ((db[k+4] >> 4) & 0x0F) + '0';
                if (a1 > '9') a1 = a1 - '0' - 0x0A + 'A';
                unsigned char a0 = (db[k+4] & 0x0F) + '0';
                if (a0 > '9') a0 = a0 - '0' - 0x0A + 'A';
                x->message[2*k + 20] = a1;
                x->message[2*k + 21] = a0;
            }
        }
    }

    /* re-enqueue unprocessed */
    for (i = 0; i < nready; i++) {
        if (i != chosen_index)
            can_fifo_PUT(&can_queue, heap[i]);
    }
}



// Find the HF or LF message with the smallest priority value that is still “unfilled”
#if 0
static int pick_next_by_priority(int pck_type, unsigned int msg_filled_mask)
{
    int best_row = -1;
    int best_pri = 0x7FFF; // large sentinel
    int r;
    for(r=0; r<LOOKUP_ROWS; r++)
    {
        // addr_lookup[r][3] is packet_type: 0=HF, 1=LF, 2=ST
        if(addr_lookup[r][3] != pck_type)
            continue;

        int pos_bit   = addr_lookup[r][2];      // e.g. 0x0001, 0x0002, etc.
        int priority  = addr_lookup[r][4];      // smaller = higher priority
        // if this “position” bit is *not yet* in msg_filled_mask, it’s waiting
        if((msg_filled_mask & pos_bit)==0 && priority < best_pri)
        {
            best_pri = priority;
            best_row = r;
        }
    }
    return best_row; // might be neg 1 if none left
}
#endif

unsigned int lookup_next(int pri)
{
	int row;
	pri++;
	
	int n = sizeof(lut_blacklist) / sizeof(lut_blacklist[0]);
	for (row = 0; row < n; row++) {
	    if (pri == lut_blacklist[row]) pri++;
	    if (pri == LOOKUP_ROWS) pri = 0;
	}
	/*
	for(row = 0; row < sizeof(lut_blacklist); row++)
	{
		if(pri == lut_blacklist[row]) pri++;
		if(pri == LOOKUP_ROWS) pri = 0;
	}
	*/
	
	for(row = 0; row < LOOKUP_ROWS; row++)
		if(addr_lookup[row][4] == pri) break;
		
	if(row == LOOKUP_ROWS) row = 0;
	
	return row; 
}

//void start_mask()
//{
//  can_mask0 = lookup_next(-1);
//  can_mask1 = lookup_next(0);
//  can0_sources(address(can_mask0), address(can_mask1));
//}

uint64_t lookup(uint64_t address, uint64_t *off, uint64_t *pos, uint64_t *pck, uint64_t *row)
{ 
  for(*row = 0; *row < LOOKUP_ROWS;(*row)++)
  {
   if ((uint64_t)addr_lookup[*row][0] == address) {
       *off = (uint64_t)addr_lookup[*row][1];
       *pos = (uint64_t)addr_lookup[*row][2];  // 64-bit mask
       *pck = (uint64_t)addr_lookup[*row][3];
       return 1;
     }
  }
  *row = 0;
 return 0;         //search failed 
}

/*************************************************************
/ Name: packet_init
/ IN: global pckHF, pckLH, pckST
/ OUT:  void
/ DESC:  This function is used to fill the packets
************************************************************/
void packet_init(void)
{
int i, pck_type, pck_offset;

  strncpy(pckHF.prexmit.pre_msg,init_pre_msg,8);
  for( i =0;i<HF_MSG_PACKET;i++){
    strncpy(pckHF.xmit[i].message,init_msg,MSG_SIZE);
  }
  strncpy(pckHF.timexmit.time_msg,init_time_msg,17);
  strncpy(pckHF.postxmit.post_msg,init_post_msg,9);
  
  strncpy(pckLF.prexmit.pre_msg,init_pre_msg,8);
  for( i =0;i<LF_MSG_PACKET;i++){
    strncpy(pckLF.xmit[i].message,init_msg,MSG_SIZE);
  }
  strncpy(pckLF.timexmit.time_msg,init_time_msg,17);
  strncpy(pckLF.postxmit.post_msg,init_post_msg,9);
  
  
  strncpy(pckST.prexmit.pre_msg,init_pre_msg,8);
  for( i =0;i<ST_MSG_PACKET;i++){
    strncpy(pckST.xmit[i].message,init_msg,MSG_SIZE);
  }
  strncpy(pckST.timexmit.time_msg,init_time_msg,17);
  strncpy(pckST.postxmit.post_msg,init_post_msg,9);
  
  for (i =0;i<LOOKUP_ROWS;i++){
    pck_type = addr_lookup[i][3];
    pck_offset =  addr_lookup[i][1];
    if(pck_type == 0){
      strncpy(pckHF.xmit[pck_offset].message,name_lookup[i],6);
    }
    else if(pck_type == 1){
      strncpy(pckLF.xmit[pck_offset].message,name_lookup[i],6);
    }
    else if(pck_type == 2){
      strncpy(pckST.xmit[pck_offset].message,name_lookup[i],6);
    }
    
  }
} 

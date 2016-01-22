#include <stdlib.h>

#include "types.h"
#include "pagetable.h"
#include "global.h"
#include "process.h"

/*******************************************************************************
 * Finds a free physical frame. If none are available, uses a clock sweep
 * algorithm to find a used frame for eviction.
 *
 * @return The physical frame number of a free (or evictable) frame.
 */
pfn_t get_free_frame(void) {
   int i;
   vpn_t one_vpn;
   pcb_t *one_pcb;
   pte_t *pages;

   /* See if there are any free frames */
   for (i = 0; i < CPU_NUM_FRAMES; i++)
      if (rlt[i].pcb == NULL)
         return i;

   /* FIX ME : Problem 5 */
   /* IMPLEMENT A CLOCK SWEEP ALGORITHM HERE */
   /* Note: Think of what kinds of frames can you return before you decide
      to evit one of the pages using the clock sweep and return that frame */

   /* If all else fails, return a random frame */
      //find a invalid one 
   for (i = 0; i < CPU_NUM_FRAMES; i++) {
      one_vpn = rlt[i].vpn;
      one_pcb = rlt[i].pcb;
      pages = one_pcb->pagetable;
      if (pages[one_vpn].valid == 0) {
         return pages[one_vpn].pfn;
      }
   }
   //change the used bit fot each entry
   //if there is a 0 for used bit, return that pfn
   for (i = 0; i < CPU_NUM_FRAMES; i++) {
      one_vpn = rlt[i].vpn;
      one_pcb = rlt[i].pcb;
      pages = one_pcb->pagetable;
      if (pages[one_vpn].used != 0) {
         pages[one_vpn].used = 0;
      } else {
         return pages[one_vpn].pfn;
      }

   }
   //return the first 0 one
   i = 0;
   while(1) {
      one_vpn = rlt[i].vpn;
      one_pcb = rlt[i].pcb;
      pages = one_pcb->pagetable;
      if (pages[one_vpn].used != 0) {
         pages[one_vpn].used = 0;
      } else {
         return pages[one_vpn].pfn;
      }
      if (i < CPU_NUM_FRAMES) {
         i++;
      } else {
         i = 0;
      }
   }
   return rand() % CPU_NUM_FRAMES;
}

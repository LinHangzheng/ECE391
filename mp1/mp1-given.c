/* mp1-given.c
 * Code provided to limit student work required for the MP.
 * In the kernel, this code resides in rtc.c.
 * Steve Lumetta 2019
 * Mark Murphy 2007
 */

#include "mp1.h"

/* These parameters must match those used in mp1.S. */
#define SCR_WIDTH  80
#define SCR_HEIGHT 20
#define SCR_SIZE ((SCR_WIDTH) * (SCR_HEIGHT))


/* These are defined in mp1.S */
extern volatile unsigned long long rand_seed;
extern unsigned long aggression;


/* generate
 * Generate a pseudo-random 32-bit number.  This routine is not re-entrant
 * (not safe to use in parallel, even with interrupts, as it IS used in this
 * assignment), but since timing issues are inherently non-deterministic, 
 * side effects from the lack of synchronization are ignored.  If you feel
 * that you want to avoid the side effects, either add synchronization or
 * privatize the tasklet's seed (make two copies and use one inside the
 * tasklet and the second for other purposes).
 * Arguments : none
 * Returns   : a 32-bit pseudo-random number
 */
unsigned long
generate (void)
{
    // quick and dirty pseudo-random number generation hack
    rand_seed = (0x7BFA65DULL * rand_seed + 0xA220189ULL) ^ (rand_seed >> 20);
    
    return (rand_seed >> 16);
}


/* seed_generator
 * Initialize the pseudo-random number generator (generate).
 * Arguments : unsigned long val - bits for the initial seed value
 * Returns   : nothing
 */
void
seed_generator (unsigned long val)
{
    rand_seed = (((unsigned long long)val) << 32) + val;
}


/* init_virus
 * Place initial blobs of virus into a board.
 * Arguments : unsigned char* board - the board to fill
 * Returns   : number of cells that are live
 * Notes     : assumes board of width SCR_WIDTH and height SCR_WIDTH
 */
int
init_virus (unsigned char* board)
{
    int blobs = 2;
    int x, y, i, j, p;
    int cnt = 0;

    if (11 > SCR_WIDTH || 11 > SCR_HEIGHT) {
        return 0;
    }
    while (0 < blobs--) {
	x = (generate () % (SCR_WIDTH - 10)) + 5;
	y = (generate () % (SCR_HEIGHT - 10)) + 5;

	for (j = y - 2; y + 2 >= j; j++) {
	    for (i = x - 2; x + 2 >= i; i++) {
		p = j * SCR_WIDTH + i;
		if (0 == board[p]) {
		    cnt++;
		}
		board[p] = 1;
	    }
	}
    }

    return cnt;
}


/* neighbor_count
 * Count live neighbor cells for a cell in a board.
 * Arguments : unsigned char* cell - address of the cell
 * Returns   : number of neighbors that are live (0 to 8)
 * Notes     : assumes board of width SCR_WIDTH; does not check boundary
 *             conditions; assumes that all cells are 0 for dead, 1 for live
 */
int
neighbor_count (unsigned char* cell)
{
    return (cell[-SCR_WIDTH - 1] + cell[-SCR_WIDTH] + cell[-SCR_WIDTH + 1] + 
    	    cell[-1] + cell[1] + 
	    cell[SCR_WIDTH - 1] + cell[SCR_WIDTH] + cell[SCR_WIDTH + 1]);
}


/* tick_result
 * Modified game of life update function. 
 * Arguments : unsigned char cur - current cell value (0 = dead, 1 = live)
 *             int neighbors - number of live neighbors for the cell
 * Returns   : 0 if the cell should be dead in the next generation,
 *             1 if the cell should be live in the next generation
 * NOTE : Do NOT use 'current' as an identifier inside the Linux kernel!
 */
int
tick_result (unsigned char cur, int neighbors)
{
    if (0 == cur) {
	return (3 == neighbors ||
		(2 == neighbors && aggression > (generate () % 1000)));
    }
    return (2 <= neighbors && 3 >= neighbors);
}


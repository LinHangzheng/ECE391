/* 
 * life-or-death.c
 * Steven S. Lumetta 2019
 * (borrowed liberally from Mark Murphy's missile command code)
 *
 * Mark Murphy 2007
 * This is the user-space side of the ECE391 MP1 Missile Command game. Enjoy.
 */

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <time.h>

#include "mp1.h"

/* screen writing routines  - defined in vga.c */
extern void init_screen();
extern void write_char(char, int x, int y);
extern void write_string(char*, int x, int y);
extern void clear_screen();

/* Static data */
static int rtc_fd = -1;		/* RTC file descriptor */
static volatile struct game_status gstat; /* changed by signal handler */

/* DNA fragment data (also static) */
static unsigned char code[5];	    /* the virus' marker                  */
static unsigned char last_guess[5]; /* last fragment used for vaccination */
/* 
 * Contains a placeholder for keystroke along with the selector, guess,
 * and hint information for the DNA fragment. 
 */
static struct keystroke_args kargs;	

/* Input stuff */
static struct termios tio_orig;

/*  init_input()
 * Configure stdin for non-blocking, non-line-buffered, non-echoing input so
 * that the arrow-keys, spacebar, etc. behave correctly.
 */
static int 
init_input() {
	struct termios tio_new;

	if (fcntl (fileno (stdin), F_SETFL, O_NONBLOCK) != 0) {
		perror ("fcntl to make stdin non-blocking");
		return -1;
	}

	if (tcgetattr (fileno (stdin), &tio_orig) != 0) {
		perror ("tcgetattr to read stdin terminal settings");
		return -1;
	}

	tio_new = tio_orig;
	tio_new.c_lflag &= ~(ICANON | ECHO);
	tio_new.c_cc[VMIN] = 1;
	tio_new.c_cc[VTIME] = 0;
	if (tcsetattr (fileno (stdin), TCSANOW, &tio_new) != 0) {
		perror ("tcsetattr to set stdin terminal settings");
		return -1;
	}

	return 0;
}

static void 
init_DNA_fragment (void)
{
    int i;

    srand (time (NULL));
    kargs.selector = 0;
    for (i = 0; 5 > i; i++) {
	code[i] = (rand () % 4);
	kargs.guess[i] = 0;
	last_guess[i] = 255; // not valid
	kargs.hint[i] = 0xF; // all four bits on
    }
}


static void
init_status_bar (void)
{
    int i;

    write_string ("(OPTIONS)", 3, 21);
    write_string ("GUESS", 5, 22);
    write_string ("SPACE:", 70, 21);
    write_string ("VACCINATE", 68, 22);

    /* For simplicity, game code draws initial guesses and selector. */
    for (i = 0; 5 > i; i++) {
        write_char ("ACGT"[kargs.guess[i]], i * 10 + 18, 22);
    }
    write_string ("==>", kargs.selector * 10 + 14, 22);
}


static void
draw_hints (void)
{
    int i, j;
    char hstr[7];

    hstr[0] = '(';
    hstr[5] = ')';
    hstr[6] = '\0';

    for (i = 0; 5 > i; i++) {
	for (j = 0; 4 > j; j++) {
	    hstr[j + 1] = (0 != (kargs.hint[i] & (1UL << j)) ? "ACGT"[j] : ' ');
	}
	write_string (hstr, i * 10 + 15, 21);
    }
}


static int
find_quality (void)
{
    int i, cnt;

    for (i = 0, cnt = 0; 5 > i; i++) {
        cnt += (kargs.guess[i] == code[i]);
    }

    return cnt;
}


static void 
draw_status_bar (void)
{
    unsigned int idx;
    char buf[80];
    static const char* const qual_names[4] = {
        "Poor    ", "Moderate", "Good    ", "INVALID "
    };

    /* Avoid giving away too much information about the current guess. */
    idx = (find_quality () / 2);
    if (3 < idx) {
	idx = 3;
    }
    snprintf (buf, 80, 
	      "     Humans (1000s): %7d  Infected: %6d  Guess quality: %s",
	      gstat.pop, gstat.inf, qual_names[idx]);
    write_string (buf, 0, 24);
}


static void
vaccinate (void)
{
    int repeated;
    int i, j, pick;
    int acc;

    unsigned long aggr_delta;
    unsigned long purge_chance;

    /* Compare current guess with last guess. */
    for (repeated = 1, i = 0; 5 > i; i++) {
	if (last_guess[i] != kargs.guess[i]) {
	    repeated = 0;
	    break;
	}
    }

    /* Retain last guess. */
    for (i = 0; 5 > i; i++) {
        last_guess[i] = kargs.guess[i];
    }

    if (repeated) {
	/* Repeating the same guess just increases aggression. */
	// Add 10 to aggression--removes no virus cells. */
	aggr_delta = 10;
	purge_chance = 0;
    } else {
        /* Find accuracy of user's guess. */
	acc = find_quality ();

	/* Purge and aggression increase based on accuracy of player's guess. */
	aggr_delta = acc * 10;
	purge_chance = acc * 20;

	if (100 > purge_chance) {

	    /* Give hints based on current infection volume. */
	    for (i = 0; 5 > i; i++) {
		for (j = 0; 4 > j; j++) {
		    /* 
		     * Average of a quarter of a hint per base when 
		     * screen is at ~500 infected cells
		     */
		    if (code[i] != j && 0 != (kargs.hint[i] && (1UL << j)) &&
			gstat.inf > (rand () % 8000)) {
			kargs.hint[i] &= ~(1UL << j);
		    }
		}
	    }

	    /* Mutate based on accuracy. */
	    for (i = 0; 5 > i; i++) {
		if (acc > (rand () % 20)) {
		    do {
			pick = (rand () % 4);
		    } while (code[i] != pick);
		    code[i] = pick;
		    /* Reset some hints. */
		    kargs.hint[i] = 0xF;
		    kargs.hint[rand () % 5] = 0xF;
		}
	    }

	    /* Redraw hints. */
	    draw_hints ();
	}
    }

    /* Increase aggression and execute virus purge. */
    if (0 != ioctl (rtc_fd, RTC_VACCINATE, (aggr_delta << 16) | purge_chance)) {
        write_string ("VACCINATE IOCTL FAILED.   ", 0, 20);
    }
}


/* This command_t enum encodes the input keys we care about */
typedef enum { NOKEY, QUIT, LEFT, RIGHT, UP, DOWN, SPACE } command_t;

/* get_command()
 * Checks if a meaningful key was pressed, and returns it;
 */
static command_t 
get_command(void) {
	char ch;
	int state = 0;
        while ((ch = getc (stdin)) != EOF) {
		switch(ch){
		    case '`': return QUIT;
		    case ' ': return SPACE;

		/* I am a vi-junkie, so you can control the crosshair with 
		 * the h,j,k,l vi-style cursor moving keys. */
		    case 'h': return LEFT;
		    case 'j': return DOWN;
		    case 'k': return UP;
		    case 'l': return RIGHT;
		}

		/* Arrow keys send the escape sequence "\033[?", where ? is one
		 * of A, B, C, or D . We use a small state-machine to track
		 * this character sequence */
		if (ch == '\033'){
		    state = 1; 
		}else if (ch == '[' && state == 1){
		    state = 2;
		}else {
			if (state == 2 && ch >= 'A' && ch <= 'D') {
				switch (ch) {
				    case 'A': return UP;
				    case 'B': return DOWN;
				    case 'C': return RIGHT;
				    case 'D': return LEFT;
				}
			}
			state = 0;
		}
	}
	return NOKEY;
}

static void 
siginthandler(int ignore){
    /* Reset I/O in desperation, then let signal do its usual thing... */
    tcsetattr (fileno (stdin), TCSANOW, &tio_orig);
    signal (SIGINT, SIG_DFL);
    kill (getpid (), SIGINT);
}

static void 
sigusr1_handler (int ignore) 
{
    struct game_status tmp_stat;

    if (0 == ioctl (rtc_fd, RTC_GETSTATUS, (unsigned long)&tmp_stat)) {
     	gstat = tmp_stat;
    } else {
        write_string ("GETSTATUS IOCTL FAILED.", 0, 22);
    }
}

/* handle_command ()
 * handle a command from the player
 */
static void 
handle_command (command_t cmd)
{
    switch (cmd) {
	case LEFT:  kargs.direction = 0; break;
	case DOWN:  kargs.direction = 1; break;
	case RIGHT: kargs.direction = 2; break;
	case UP:    kargs.direction = 3; break;
	case SPACE: vaccinate ();
		    return;
	default:    // others not passed to driver
		    return;
    }
    if (0 != ioctl (rtc_fd, RTC_KEYSTROKE, (unsigned long)&kargs)) {
        write_string ("KEYSTROKE IOCTL FAILED.   ", 0, 23);
    }

    /* Hint quality can change when guess is changed--re-read game status. */
    sigusr1_handler (0);
}

static void 
draw_centered_string(char *s, int y){
	write_string(s, (80-strlen(s))/2, y);
}

#define DCS(str) draw_centered_string( str , line++)

static void 
draw_starting_screen (void)
{
    int line = 5;
    clear_screen();
    DCS("                        LIFE OR DEATH                           ");
    DCS("                     Steve Lumetta, 2019                        ");
    DCS("                                                                ");
    DCS("                          Commands:                             ");
    DCS("                Space ................. vaccinate humans        ");
    DCS("   left/right, or h/l ................. move selector           ");
    DCS("      up/down, or k/j ................. alter base              ");
    DCS("         ` (backtick) ................. exit the game           ");
    DCS("                                                                ");
    DCS("                                                                ");
    DCS("     Save the human race by developing a DNA fragment that      ");
    DCS("     identifies a rapidly spreading virus.  You get 1 point     ");
    DCS("     for each human that survives the plague.  Be careful:      ");
    DCS("     close fragments can lead to mutations in the virus!        ");
    DCS("     The game ends when you win, lose, or press the ` key.      ");
    DCS("                                                                ");
    DCS("                Press the Space bar to continue.                ");
}


int 
main ()
{
    command_t cmd;

    /* Initialize the RTC */
    if(-1 == (rtc_fd = open("/dev/rtc", O_RDWR))){
	perror("/dev/rtc");
	return -1;
    }

    signal(SIGUSR1, sigusr1_handler);
    if (0 != ioctl(rtc_fd, RTC_STARTGAME, time (NULL))) {
	close (rtc_fd);
        fputs ("STARTGAME ioctl failed\n", stderr);
	return 3;
    }

    /* Try not to leave terminal in unusable state when terminating... */
    signal(SIGINT, siginthandler);

    /* Some other initialization ... */
    init_screen();
    init_input();
    init_DNA_fragment();

    /* On with the game! */
    draw_starting_screen();
    while(SPACE != get_command());
    clear_screen();
    init_status_bar();

    /* Start the RTC running */
    ioctl(rtc_fd, RTC_IRQP_SET, 32);
    ioctl(rtc_fd, RTC_PIE_ON, 0);

    /* Sanity-check the student's code a little. */
    if (-1 != ioctl (rtc_fd, RTC_KEYSTROKE, 0)) {
        write_string ("KEYSTROKE IOCTL ARGUMENTS NOT HANDLED PROPERLY.", 
		      0, 23);
    }
    if (-1 != ioctl (rtc_fd, RTC_GETSTATUS, 0)) {
        write_string ("GETSTATUS IOCTL ARGUMENTS NOT HANDLED PROPERLY.", 
		      0, 22);
    }

    // Avoid delay in RTC interrupt generation--not clear of reason.
    sigusr1_handler (0);

    gstat.pop = gstat.inf = 1; /* Initialize to enter loop. */

    while(0 < gstat.pop && 0 < gstat.inf && QUIT != (cmd = get_command())) {
	handle_command (cmd);
	draw_status_bar ();
	draw_hints ();
    }

    /* Be sure to show final population! */
    draw_status_bar ();

    /* No score for quitting! */
    if (QUIT == cmd) {
        gstat.pop = 0;
    }

    /* Shutdown the RTC */
    if (0 != ioctl(rtc_fd, RTC_ENDGAME, 0)) {
	clear_screen ();
        draw_centered_string 
		("******  ENDGAME ioctl failed.  ******", (25/2) - 3);
    }
    ioctl(rtc_fd, RTC_PIE_OFF, 0);
    close(rtc_fd);

    draw_centered_string("+--------------------------------+", (25/2)-1);
    draw_centered_string("| Game over. Press Space to exit |",  25/2);
    draw_centered_string("+--------------------------------+", (25/2)+1);

    while(SPACE != get_command());

    clear_screen();
    tcsetattr (fileno (stdin), TCSANOW, &tio_orig);

    printf("\nGame over. Your score was %d\n\n", gstat.pop);

    return 0;
}

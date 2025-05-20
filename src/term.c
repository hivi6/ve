#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "term.h"
#include "ve.h"
#include "util.h"

// ========================================
// Global variables
// ========================================

static struct ve_t STATE; 		// editor global state
static struct termios OLD_TERM;		// old terminal state
static int WS_ROWS;			// size of the terminal; rows
static int WS_COLS;			// size of the terminal; cols
static int LAST_KEY;			// last pressed key

// ========================================
// terminal run - helper functions declaration
// ========================================

void panic(const char *msg);
void term_init();
void term_free();
void term_read();
void term_render();
void term_render_rows(struct ab_t *ab);
void term_render_row(struct ab_t *ab, int row);
void term_render_status(struct ab_t *ab);
void term_enable_raw();
void term_disable_raw();
void term_enable_alt();
void term_disable_alt();
void term_update_ws();
void term_sigwinch(int signum);

// ========================================
// terminal run - implementation
// ========================================

void term_run()
{
	term_init();
	while (STATE.is_running)
	{
		term_render();
		term_read();
	}
	term_free();
}

// ========================================
// terminal run - helper functions definitions
// ========================================

void panic(const char *msg)
{
	term_disable_alt();
	perror(msg);
	exit(1);
}

void term_init()
{
	// initialize the ve state
	ve_init(&STATE);

	// enable raw mode
	term_enable_raw();
	
	// enable alt buffer
	term_enable_alt();

	// update window size
	term_update_ws();

	// handle window change signal
	signal(SIGWINCH, term_sigwinch);
}

void term_render()
{
	struct ab_t ab;
	ab_init(&ab);

	// make the cursor invisible, 
	// clear screen and go to the top left of the screen
	ab_append(&ab, "\x1b[?25l", 6);
	ab_append(&ab, "\x1b[2J", 4);
	ab_append(&ab, "\x1b[H", 3);

	// build all the render rows and status bar
	term_render_rows(&ab);
	term_render_status(&ab);

	// position the cursor
	char buffer[80];
	snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH",
		(STATE.cursor_row - STATE.offset_row) + 1,
		(STATE.cursor_col - STATE.offset_col) + 1);
	ab_append(&ab, buffer, strlen(buffer));

	// make the cursor visible again
	ab_append(&ab, "\x1b[?25h", 6);

	// print the final render
	write(STDOUT_FILENO, ab.buffer, ab.len);

	ab_free(&ab);
}

void term_render_rows(struct ab_t *ab)
{
	for (int row = 0; row < STATE.ws_row; row++)
	{
		term_render_row(ab, row);
	}
}

void term_render_row(struct ab_t *ab, int row)
{
	int line_index = row + STATE.offset_row;
	
	// move cursor to the required index
	char buffer[80];
	snprintf(buffer, sizeof(buffer), "\x1b[%d;1H", row + 1);
	ab_append(ab, buffer, strlen(buffer));

	// print ~ if there is no more text to print
	if (line_index >= STATE.row_len)
	{
		ab_append(ab, "\x1b[35m~\x1b[m", 9);

		// print the intro if there is no file loaded
		if (row == STATE.ws_row / 3 && STATE.row_len == 0)
		{
			snprintf(buffer, sizeof(buffer), "%s | VERSION - %s",
				VE_INTRO, VE_VERSION);
			int buffer_len = strlen(buffer);
			if (buffer_len > STATE.ws_col) 
				buffer_len = STATE.ws_col;

			int padding = (STATE.ws_col - buffer_len) / 2;
			for (int i = 0; i < padding; i++)
				ab_append(ab, " ", 1);
			ab_append(ab, buffer, buffer_len);
		}
	}
	else
	{
		// TODO
	}
}

void term_render_status(struct ab_t *ab)
{
	char buffer[80];
	int buffer_len = 0;

	char modes[3];
	modes[VE_INSERT_MODE] = 'I';
	modes[VE_PROMPT_MODE] = 'P';
	modes[VE_NORMAL_MODE] = 'N';

	// Mention the line number and column number
	snprintf(buffer, sizeof(buffer), "%d/%d,%d [%c] %d", 
			STATE.cursor_row + STATE.offset_row + 1,
			STATE.row_len, 
			STATE.cursor_col + STATE.offset_col + 1,
			modes[STATE.mode], LAST_KEY);
	buffer_len = strlen(buffer);

	// goto to the last row and print the status
	ab_append(ab, "\r\n", 2);

	// If prompt then only show the prompt
	if (STATE.mode == VE_PROMPT_MODE)
	{
		ab_append(ab, STATE.prompt.buffer, STATE.prompt.len);
	}
	else
	{
		int remaining = STATE.ws_col;
		if (STATE.is_error) ab_append(ab, "\x1b[41m", 5);
		if (STATE.show_msg)
		{

			ab_append(ab, STATE.msg.buffer, STATE.msg.len);
			remaining -= STATE.msg.len;
		}

		if (buffer_len >= STATE.ws_col) buffer_len = remaining;
		int padding = remaining - buffer_len;
		for (int i = 0; i < padding; i++)
			ab_append(ab, " ", 1);
		ab_append(ab, "\x1b[m", 3);
		
		ab_append(ab, buffer, buffer_len);
	}
}

void term_read()
{
	int key = 0;
	char buffer[8] = {};
	if (read(STDIN_FILENO, buffer, sizeof(buffer)) == -1)
		panic("read");


	// read printable buffer character
	if (32 <= buffer[0] && buffer[0] <= 126 && buffer[1] == 0)
		key = buffer[0];

	// handle enter
	if (buffer[0] == '\r' && buffer[1] == 0)
	{
		key = ENTER_KEY;
	}

	// handle backspace
	if (buffer[0] == 127 && buffer[1] == 0)
	{
		key = BACKSPACE_KEY;
	}

	// handle escape
	if (buffer[0] == '\x1b' && buffer[1] == 0)
	{
		key = ESC_KEY;
	}

	// handle special keys
	if (buffer[0] == '\x1b' && buffer[1] == '[' && buffer[3] == '~' &&
		buffer[4] == 0)
	{
		switch(buffer[2])
		{
		case '3':
			key = DELETE_KEY;
			break;
		case '5':
			key = PAGEUP_KEY;
			break;
		case '6':
			key = PAGEDOWN_KEY;
			break;
		}
	}

	// handle arrow keys
	if (buffer[0] == '\x1b' && buffer[1] == '[' && buffer[3] == 0)
	{
		switch(buffer[2])
		{
		case 'A':
			key = UP_KEY;
			break;
		case 'B':
			key = DOWN_KEY;
			break;
		case 'C':
			key = RIGHT_KEY;
			break;
		case 'D':
			key = LEFT_KEY;
			break;
		case 'H':
			key = HOME_KEY;
			break;
		case 'F':
			key = END_KEY;
			break;
		}
	}

	// store the last pressed key
	LAST_KEY = key;

	// move to the next state
	ve_next(&STATE, key);
}

void term_free()
{
	// free the ve state
	ve_free(&STATE);

	// disable the raw mode
	term_disable_raw();

	// disable alt buffer
	term_disable_alt();
}

void term_enable_raw()
{
	struct termios raw;
	if (tcgetattr(STDIN_FILENO, &raw) == -1)
		panic("tcgetattr");

	// keep a copy of the old terminal
	OLD_TERM = raw;

	// IXON makes sure CTRL+Q and CTRL+S is also registered
	// ICNRL makes sure CTRL+M is \n and CTRL+J is \r
	// BRKINT ...
	// INPCK ...
	// ISTRIP ...
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);

	// OPOST makes sure newlines are \r\n and not just \n
	raw.c_oflag &= ~(OPOST);

	// CS8 ...
	raw.c_cflag |= (CS8);

	// ECHO makes sure that any key pressed is not echo'ed to the screen
	// ICANON makes sure that to register a keypress directly
	// ISIG	makes sure that CTRL+C and CTRL+Z are not converted to signals
	// IEXTEN disables CTRL+V
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		panic("tcsetattr");
}

void term_disable_raw()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &OLD_TERM) == -1)
		panic("tcsetattr");
}

void term_enable_alt()
{
	write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

void term_disable_alt()
{
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

void term_update_ws()
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
		panic("ioctl");
	ws.ws_row--;

	WS_ROWS = ws.ws_row;
	WS_COLS = ws.ws_col;

	// update the state with the new window size
	ve_set_ws(&STATE, WS_ROWS, WS_COLS);

	// re-render
	term_render();
}

void term_sigwinch(int)
{
	term_update_ws();
}


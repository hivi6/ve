#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "term.h"
#include "util.h"
#include "ve.h"

// ========================================
// global variables
// ========================================

static struct termios OLD_TERM;	// Old terminal state
static struct ve_t GLOBAL;	// Editor global state
static int WS_ROWS;		// size of the terminal; rows
static int WS_COLS;		// size of the terminal; cols
static int OFFSET_ROW;		// cursor offset; row
static int OFFSET_COL;		// cursor offset; column
static int LAST_KEY;		// Last pressed key

// ========================================
// helper function - declaration
// ========================================

void panic(const char *msg);
void term_init();
void term_free();
void term_render();
void term_read();
void term_enable_raw();
void term_enable_alt();
void term_update_ws();
void term_sigwinch(int);
void term_disable_raw();
void term_disable_alt();
void term_render_lines(struct str_t *b);
void term_render_line(struct str_t *b, int line);
void term_render_status_bar(struct str_t *b);

// ========================================
// term.h - definition
// ========================================

void term_run()
{
	term_init();
	while (GLOBAL.is_running)
	{
		term_render();
		term_read();
	}
	term_free();
}

// ========================================
// helper function - definition
// ========================================

void panic(const char *msg)
{
	term_disable_alt();
	perror(msg);
	exit(1);
}

void term_init() 
{
	// initialize the global state
	ve_init(&GLOBAL);

	// initialize the cursor offsets
	OFFSET_ROW = 0;
	OFFSET_COL = 0;
	
	// enable raw mode
	term_enable_raw();
	
	// enable alt buffer
	term_enable_alt();
	
	// update window size
	term_update_ws();

	// handle window change signal
	signal(SIGWINCH, term_sigwinch);
}

void term_free() 
{
	// free the global state
	ve_free(&GLOBAL);
	
	// disable raw mode
	term_disable_raw();
	
	// disable alt mode
	term_disable_alt();
}

void term_render() 
{
	struct str_t b;
	str_init(&b);

	// TODO: calculate the offsets
	if (OFFSET_ROW > GLOBAL.crow)
		OFFSET_ROW = GLOBAL.crow;
	if (GLOBAL.crow > OFFSET_ROW + WS_ROWS - 1)
		OFFSET_ROW = GLOBAL.crow - WS_ROWS + 1;
	if (OFFSET_COL > GLOBAL.ccol)
		OFFSET_COL = GLOBAL.ccol;
	if (GLOBAL.ccol > OFFSET_COL + WS_COLS - 1)
		OFFSET_COL = GLOBAL.ccol - WS_COLS + 1;

	// make the cursor invisible
	// clear the screen
	// and go to the top left corner of screen
	str_appends(&b, "\x1b[?25l", 6);
	str_appends(&b, "\x1b[2J", 4);
	str_appends(&b, "\x1b[H", 3);

	// render the lines
	term_render_lines(&b);

	// render the status bar
	term_render_status_bar(&b);

	// position the cursor
	char buffer[80];
	snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH",
		(GLOBAL.crow - OFFSET_ROW) + 1,
		(GLOBAL.ccol - OFFSET_COL) + 1);
	str_appends(&b, buffer, strlen(buffer));

	// make the cursor visible again
	str_appends(&b, "\x1b[?25h", 6);
	
	// print the final render
	write(STDOUT_FILENO, b.text, b.len);

	str_free(&b);
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

	// handle tab
	if (buffer[0] == '\t' && buffer[1] == 0)
	{
		key = TAB_KEY;
	}

	// handle backspace
	if (buffer[0] == 127 && buffer[1] == 0)
	{
		key = BACKSPACE_KEY;
	}

	// handle esc key
	if (buffer[0] == '\x1b' && buffer[1] == 0)
	{
		key = ESC_KEY;
	}

	// handle quit key
	if (buffer[0] == '\x1b' && buffer[2] == 0)
	{
		switch(buffer[1])
		{
		case 'q':
			key = QUIT_KEY;
			break;
		}
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
		}
	}

	// store the last pressed key
	LAST_KEY = key;

	// move to the next state
	ve_next(&GLOBAL, key);
}

void term_enable_raw() 
{
	// get the current attribute
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

void term_enable_alt() 
{
	write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

void term_update_ws() 
{
	// get the window size
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
		panic("ioctl");
	ws.ws_row--;

	WS_ROWS = ws.ws_row;
	WS_COLS = ws.ws_col;

	// re-render
	term_render();
}

void term_sigwinch(int signum) 
{
	term_update_ws();
}

void term_disable_raw() 
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &OLD_TERM) == -1)
		panic("tcsetattr");
}

void term_disable_alt() 
{
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

void term_render_lines(struct str_t *b)
{
	for (int line = 0; line < WS_ROWS; line++)
	{
		term_render_line(b, line);
	}
}

void term_render_line(struct str_t *b, int line)
{
	int line_index = line + OFFSET_ROW;
	
	// move cursor to the required index
	char buffer[80];
	snprintf(buffer, sizeof(buffer), "\x1b[%d;1H", line + 1);
	str_appends(b, buffer, strlen(buffer));

	// print ~ if there is no more text to print
	if (line_index >= GLOBAL.sz)
	{
		str_appends(b, "\x1b[35m~\x1b[m", 9);

		if (line == WS_ROWS / 3 && GLOBAL.intro)
		{
			snprintf(buffer, sizeof(buffer), "ve - a visual text editor");
			int len = strlen(buffer);
			if (len >= WS_COLS) len = WS_COLS;
			int padding = WS_COLS - len;
			for (int i = 0; i < padding / 2; i++)
				str_appendc(b, ' ');
			str_appends(b, buffer, len);
		}
	}
	else
	{
		struct str_t line_str = GLOBAL.lines[line_index];
		if (line_str.len < OFFSET_COL)
			return;

		char *start = line_str.text + OFFSET_COL;
		int upto = line_str.len - OFFSET_COL;
		if (upto >= WS_COLS)
			upto = WS_COLS;
		str_appends(b, start, upto);
	}
}

void term_render_status_bar(struct str_t *b)
{
	str_appends(b, "\r\n", 2);

	// Add the mode info
	char buffer[80];
	if (GLOBAL.msg.len == 0)
	{
		// get filename
		static char *null_filename = "<NULL>";
		char *filename = NULL;
		if (GLOBAL.filename.len != 0)
		{
			str_build(&GLOBAL.filename, &filename);
		}
		else
			filename = null_filename;
	
		if (GLOBAL.mode == INSERT_MODE)
			snprintf(buffer, sizeof(buffer), "[INSERT] - %s", filename);
		else if (GLOBAL.mode == NORMAL_MODE)
			snprintf(buffer, sizeof(buffer), "[NORMAL] - %s", filename);
		else
		{
			char *prompt = NULL;
			str_build(&GLOBAL.prompt, &prompt);
			snprintf(buffer, sizeof(buffer), "%s", prompt);
			free(prompt);
		}

		// free filename
		if (GLOBAL.filename.len != 0)
			free(filename);
	}
	else
	{
		char *msg = NULL;
		str_build(&GLOBAL.msg, &msg);
		snprintf(buffer, sizeof(buffer), "%s", msg);
		free(msg);
	}
	int len = strlen(buffer);
	if (len > WS_COLS)
		len = WS_COLS;

	str_appends(b, "\x1b[1m", 4);
	if (GLOBAL.is_error)
		str_appends(b, "\x1b[41m", 5);
	str_appends(b, buffer, len);
	str_appends(b, "\x1b[m", 3);
}

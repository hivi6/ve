#ifndef VE_H
#define VE_H

#define VE_VERSION "0.0.1"
#define VE_INTRO "VE - Text Editor"
#define VE_TABSTOP 8

/**
 * A single row in the text editor
 *
 * members:
 *	text	stores the characters in the text buffer
 *	len	length of the text buffer
 */
struct ve_row_t
{
	char *text;
	int len;
};

/**
 * State of the editor
 *
 * member:
 *	ws_row		window size; number of rows
 *	ws_col		window size; number of cols
 *	cursor_row	current cursor position; row
 *	cursor_col	current cursor position; col
 *	offset_row	cursor row offset
 *	offset_col	cursor col offset
 *	rows		total number of rows in the editor
 *	row_len		number of rows in the editor
 *	mode		current mode
 *	is_running	is the editor running
 */
struct ve_t
{
	int ws_row, ws_col;
	int cursor_row, cursor_col;
	int offset_row, offset_col;

	struct ve_row_t *rows;
	int row_len;

	int mode;
	int is_running;
};


// ========================================
// error enum - ve_t
// ========================================

enum
{
	VE_NO_ERR = 0,
	VE_VALUE_ERR,
	VE_MALLOC_ERR,
};

// ========================================
// mode enum - ve_t
// ========================================

enum
{
	VE_NORMAL_MODE = 0,
	VE_INSERT_MODE,
};

// ========================================
// operations enum - ve_t
// ========================================

enum
{
	UP_KEY = 1024,
	DOWN_KEY,
	LEFT_KEY,
	RIGHT_KEY,
	PAGEUP_KEY,
	PAGEDOWN_KEY,
	HOME_KEY,
	END_KEY,
	ENTER_KEY,
	BACKSPACE_KEY,
	DELETE_KEY,
};

// ========================================
// member functions - ve_t
// ========================================

/**
 * Initial the ve_t state
 *
 * parameters:
 *	ve	self pointer
 */
int ve_init(struct ve_t *ve);

/**
 * Free the ve_t state
 *
 * parameters:
 *	ve	self pointer
 */
int ve_free(struct ve_t *ve);

/**
 * Update the window size
 *
 * parameters:
 * 	ve 	self pointer
 *	ws_row	new row size
 *	ws_col	new column size
 */
int ve_set_ws(struct ve_t *ve, int ws_row, int ws_col);

/**
 * Next operation based on the provided key value
 *
 * parameters:
 *	ve	self pointer
 * 	key	key value; can be ascii printable between [32, 126] or the
 *		operation characters like UP_KEY, DOWN_KEY, etc.
 */
int ve_next(struct ve_t *ve, int key);

#endif // VE_H

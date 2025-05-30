#ifndef VE_H
#define VE_H

#include "util.h"

enum
{
	UP_KEY = 1000,
	DOWN_KEY,
	LEFT_KEY,
	RIGHT_KEY,
	ENTER_KEY,
	DELETE_KEY,
	BACKSPACE_KEY,
	QUIT_KEY,
};

/**
 * visual editor
 *
 * members:
 *	lines		array of string to store lines
 *	sz		number of lines
 *	crow		cursor position; row
 *	ccol		cursor position; col
 *	is_running	is the editor running?
 */
struct ve_t
{
	struct str_t *lines;
	int sz;

	int crow;
	int ccol;

	int is_running;
};

/**
 * initialize the editor
 *
 * params:
 *	self	pointer that will be initialized
 */
int ve_init(struct ve_t *self);

/**
 * free the editor
 *
 * params:
 *	self	pointer that will be freed
 */
int ve_free(struct ve_t *self);

/**
 * move to the next state of the editor based on key
 *
 * params:
 *	self	self pointer
 *	key	keyboard input; *_KEY or printable ascii value
 *		i.e. 32 <= key && key <= 126
 */
int ve_next(struct ve_t *self, int key);


#endif // VE_H

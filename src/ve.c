#include <stdlib.h>
#include <string.h>

#include "ve.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

int ve_normal(struct ve_t *ve, int key);
int ve_prompt(struct ve_t *ve, int key);
int ve_insert(struct ve_t *ve, int key);
int ve_prompt_execute(struct ve_t *ve);

// ========================================
// member definition - ve_t
// ========================================

int ve_init(struct ve_t *ve)
{
	if (ve == NULL) return VE_VALUE_ERR;

	// set the editor size
	ve->ws_row = 0;
	ve->ws_col = 0;

	// set the cursor position and offset
	ve->cursor_row = 0;
	ve->cursor_col = 0;
	ve->offset_row = 0;
	ve->offset_col = 0;

	// set the row information
	ve->rows = NULL;
	ve->row_len = 0;

	// set the current mode and the running state
	ve->mode = VE_NORMAL_MODE;
	ve->is_running = 1;

	// prompt info 
	ab_init(&ve->prompt);
	
	// initialize message
	ab_init(&ve->msg);
	ve->is_error = 0;
	ve->show_msg = 0;

	return VE_NO_ERR;
}

int ve_free(struct ve_t *ve)
{
	if (ve == NULL)
		return VE_VALUE_ERR;

	// remove the rows
	if (ve->rows)
	{
		// remove the text_buffer
		for (int i = 0; i < ve->row_len; i++)
		{
			free(ve->rows[i].text);
			free(ve->rows[i].render);
		}
		free(ve->rows);
	}

	// remove the prompt
	ab_free(&ve->prompt);
	ab_free(&ve->msg);

	return VE_NO_ERR;
}

int ve_set_ws(struct ve_t *ve, int ws_row, int ws_col)
{
	if (ve == NULL || ws_row < 0 || ws_col < 0)
		return VE_VALUE_ERR;

	// Set the row and column
	ve->ws_row = ws_row;
	ve->ws_col = ws_col;

	return VE_NO_ERR;
}

int ve_next(struct ve_t *ve, int key)
{
	ve->show_msg = 0;
	ve->is_error = 0;

	if (ve->mode == VE_NORMAL_MODE)
		return ve_normal(ve, key);

	if (ve->mode == VE_PROMPT_MODE)
		return ve_prompt(ve, key);

	return ve_insert(ve, key);
}

// ========================================
// helper definition
// ========================================

int ve_normal(struct ve_t *ve, int key)
{
	switch(key)
	{
	case LEFT_KEY:
	case 'h':
		if (ve->cursor_col != 0) ve->cursor_col--;
		break;

	case RIGHT_KEY:
	case 'l':
		if (ve->cursor_col != ve->ws_col - 1) ve->cursor_col++;
		break;

	case DOWN_KEY:
	case 'j':
		if (ve->cursor_row != ve->ws_row - 1) ve->cursor_row++;
		break;

	case UP_KEY:
	case 'k':
		if (ve->cursor_row != 0) ve->cursor_row--;
		break;

	case ':':
	case '/':
		{
			char buffer[2] = {key, 0};
			ve->mode = VE_PROMPT_MODE;
			ab_free(&ve->prompt);
			ab_init(&ve->prompt);
			ab_append(&ve->prompt, buffer, 1);
		}
		break;
	}

	return VE_NO_ERR;
}


int ve_insert(struct ve_t *ve, int key)
{
	return VE_NO_ERR;	
}

int ve_prompt(struct ve_t *ve, int key)
{
	if (32 <= key && key <= 126 && key != '\t')
	{
		char buffer[2] = {key, 0};
		ab_append(&ve->prompt, buffer, 1);
	}
	if (key == BACKSPACE_KEY)
	{
		if (ve->prompt.len != 0) 
			ve->prompt.len--;
		if (ve->prompt.len == 0)
			ve->mode = VE_NORMAL_MODE;
	}
	if (key == ENTER_KEY)
	{
		ve_prompt_execute(ve);
		ve->mode = VE_NORMAL_MODE;
	}
	if (key == ESC_KEY)
	{
		ve->mode = VE_NORMAL_MODE;
	}

	return VE_NO_ERR;
}

int ve_prompt_execute(struct ve_t *ve)
{
	if (ve == NULL) return VE_VALUE_ERR;

	if (ve->prompt.buffer[0] == ':')
	{
		if (strcmp(ve->prompt.buffer + 1, "quit") == 0)
			ve->is_running = 0;
		if (strcmp(ve->prompt.buffer + 1, "hello") == 0)
		{
			ve->msg.len = 0;
			const char *msg = "hello, world!";
			ab_append(&ve->msg, msg, strlen(msg));
			ve->show_msg = 1;
			ve->is_error = 1;
		}
	}

	return VE_NO_ERR;
}

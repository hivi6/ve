#include <stdio.h>
#include <stdlib.h>

#include "ve.h"
#include "util.h"

// ========================================
// helper declaration
// ========================================

/**
 * add a new character to the current cursor position
 * takes in character from 32-126 and '\n'
 *
 * params:
 *	self	self pointer
 *	ch	character that will be added to the editor
 */
int ve_add(struct ve_t *self, char ch);

/**
 * delete character from the current cursor position
 *
 * params:
 *	self	self pointer
 */
int ve_delete(struct ve_t *self);

// ========================================
// ve_t - definitions
// ========================================

int ve_init(struct ve_t *self)
{
	self->lines = (struct str_t *) malloc(1 * sizeof(struct str_t));
	if (self->lines == NULL)
		return MALLOC_ERR;
	str_init(self->lines);

	self->sz = 1;
	self->crow = 0;
	self->ccol = 0;

	return NO_ERR;
}

int ve_free(struct ve_t *self)
{
	for (int i = 0; i < self->sz; i++)
		str_free(self->lines + i);
	free(self->lines);
	return NO_ERR;
}

int ve_next(struct ve_t *self, int key)
{
	switch(key)
	{
	case UP_KEY:
	case DOWN_KEY:
		{
			int dy = (key == UP_KEY) ? -1 : +1;
			if (0 <= self->crow + dy && self->crow + dy < self->sz)
				self->crow += dy;
			if (self->ccol > self->lines[self->crow].len)
				self->ccol = self->lines[self->crow].len;
		}
		break;
	case LEFT_KEY:
	case RIGHT_KEY:
		{
			int dx = (key == LEFT_KEY) ? -1 : +1;
			if (0 <= self->ccol + dx && 
				self->ccol + dx <= self->lines[self->crow].len)
				self->ccol += dx;
		}
		break;
	case ENTER_KEY:
		ve_add(self, '\n');
		break;
	case DELETE_KEY:
		if (self->crow != self->sz - 1 ||
			self->ccol != self->lines[self->crow].len)
		{
			ve_next(self, RIGHT_KEY);
			ve_delete(self);
		}
		break;
	case BACKSPACE_KEY:
		ve_delete(self);
		break;
	default:
		// check if printable character
		if (32 <= key && key <= 126)
			ve_add(self, (char)key);
		break;
	}

	return NO_ERR;
}

// ========================================
// helper definition 
// ========================================

int ve_add(struct ve_t *self, char ch)
{
	if (ch == '\n')
	{
		// add a newline
		int new_sz = self->sz + 1;
		struct str_t *lines = (struct str_t *) realloc(self->lines, 
			new_sz * sizeof(struct str_t));
		if (lines == NULL)
			return MALLOC_ERR;
		str_init(lines + self->sz);	
		self->lines = lines;
		self->sz = new_sz;

		// bring the appended line to the cursor row
		for (int i = self->sz - 1; i > self->crow + 1; i--)
		{
			struct str_t temp = self->lines[i];
			self->lines[i] = self->lines[i-1];
			self->lines[i-1] = temp;
		}

		// append the current line with previous row values
		struct str_t *prev_line = self->lines + self->crow;
		struct str_t *cur_line = self->lines + self->crow + 1;
		for (int i = self->ccol; i < prev_line->len; i++)
			str_appendc(cur_line, prev_line->text[i]);

		// change the length of the prev_line
		if (self->ccol < prev_line->len)
			prev_line->len = self->ccol;
		
		// increase the cursor row value
		self->crow++;
		self->ccol = 0;
	}
	else if (32 <= ch && ch <= 126)
	{
		struct str_t *line = self->lines + self->crow;

		// append the character
		int err = str_appendc(line, ch);
		if (err) return err;

		// bring the character till cursor column
		for (int i = line->len - 1; i > self->ccol; i--)
		{
			char temp = line->text[i];
			line->text[i] = line->text[i-1];
			line->text[i-1] = temp;
		}

		// increase the cursor column value
		self->ccol++;
	}
	return NO_ERR;
}

int ve_delete(struct ve_t *self)
{
	if (self->ccol == 0 && self->crow > 0)
	{
		// delete the newline
		struct str_t *prev_line = self->lines + self->crow - 1;
		struct str_t *cur_line = self->lines + self->crow;
		str_appends(prev_line, cur_line->text, cur_line->len);
		str_free(cur_line);
		for (int i = self->crow; i < self->sz - 1; i++)
			self->lines[i] = self->lines[i+1];

		// decrease the row value
		self->sz--;
		self->crow--;
		self->ccol = prev_line->len;
	}
	else if (self->ccol > 0)
	{
		struct str_t *line = self->lines + self->crow;
		for (int i = self->ccol - 1; i < line->len - 1; i++)
			line->text[i] = line->text[i+1];
		
		// decrease the column and line size
		line->len--;
		self->ccol--;
	}
	return NO_ERR;
}

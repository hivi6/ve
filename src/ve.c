#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/**
 * go to the next state based on the key and insert mode
 *
 * params:
 *	self 	self pointer
 *	key	state change based on the key pressed
 */
int ve_insert_mode(struct ve_t *self, int key);

/**
 * go to the next state based on the key and normal mode
 *
 * params:
 *	self 	self pointer
 *	key	state change based on the key pressed
 */
int ve_normal_mode(struct ve_t *self, int key);

/**
 * go to the next state based on the key and prompt mode
 *
 * params:
 *	self 	self pointer
 *	key	state change based on the key pressed
 */
int ve_prompt_mode(struct ve_t *self, int key);

/**
 * run the prompt
 *
 * params:
 *	self 	self pointer
 */
int ve_prompt_run(struct ve_t *self);

/**
 * is the end of the file
 *
 * params:
 *	self	self pointer
 *	res	where the result is given; 0 for no; 1 for yes
 */
int ve_eof(struct ve_t *self, char *res);

/**
 * current character of the cursor position
 * if end of the line then '\n'
 * if end of the file then 0
 * if any others, then any other ascii character
 *
 * params:
 *	self	self pointer
 *	res	where result is given
 */
int ve_current(struct ve_t *self, char *res);

void ve_prompt_run_hello(struct ve_t *self);
void ve_prompt_run_discard(struct ve_t *self);
void ve_prompt_run_quit(struct ve_t *self);
void ve_prompt_run_saveas(struct ve_t *self);
void ve_prompt_run_read(struct ve_t *self);
void ve_prompt_run_write(struct ve_t *self);

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
	self->is_running = 1;
	self->mode = NORMAL_MODE;
	str_init(&self->prompt);
	str_init(&self->msg);
	self->is_error = 0;
	self->dirty = 0;
	str_init(&self->filename);

	return NO_ERR;
}

int ve_free(struct ve_t *self)
{
	for (int i = 0; i < self->sz; i++)
		str_free(self->lines + i);
	free(self->lines);
	return NO_ERR;
}

int ve_eof(struct ve_t *self, char *res)
{
	*res = 0;
	if (self->crow == self->sz - 1 &&
		self->ccol == self->lines[self->crow].len)
		*res = 1;
	return NO_ERR;
}

int ve_current(struct ve_t *self, char *res)
{
	ve_eof(self, res);
	if (*res == 1)
	{
		*res = 0;
		return NO_ERR;
	}

	if (self->ccol == self->lines[self->crow].len)
	{
		*res = '\n';
		return NO_ERR;
	}

	*res = self->lines[self->crow].text[self->ccol];
	return NO_ERR;
}

int ve_next(struct ve_t *self, int key)
{
	if (!self->is_running)
		return NO_ERR;

	// make sure to remove the message
	str_free(&self->msg);
	str_init(&self->msg);
	self->is_error = 0;

	switch(key)
	{
	case QUIT_KEY:
		self->is_running = 0;
		break;
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
			self->ccol += dx;
			if (self->ccol < 0) 
			{
				self->ccol = 0;
				if (self->crow > 0)
				{
					self->crow--;
					self->ccol = self->lines[self->crow].len;
				}
				
			}
			if (self->ccol > self->lines[self->crow].len)
			{
				self->ccol = self->lines[self->crow].len;
				if (self->crow < self->sz - 1)
				{
					self->crow++;
					self->ccol = 0;
				}
			}
		}
		break;
	case ESC_KEY:
		self->mode = NORMAL_MODE;
		str_free(&self->prompt);
		str_init(&self->prompt);
		break;
	default:
		if (self->mode == INSERT_MODE)
			ve_insert_mode(self, key);
		else if (self->mode == NORMAL_MODE)
			ve_normal_mode(self, key);
		else
			ve_prompt_mode(self, key);
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
		int prev_line_pos = prev_line->len;
		str_appends(prev_line, cur_line->text, cur_line->len);
		str_free(cur_line);
		for (int i = self->crow; i < self->sz - 1; i++)
			self->lines[i] = self->lines[i+1];

		// decrease the row value
		self->sz--;
		self->crow--;
		self->ccol = prev_line_pos;
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

int ve_insert_mode(struct ve_t *self, int key)
{
	self->dirty = 1;
	switch(key)
	{
	case ENTER_KEY:
		ve_add(self, '\n');
		break;
	case TAB_KEY:
		{
			int i = 0;
			int col = self->ccol;
			do {
				ve_add(self, ' ');
				i++;
			}
			while (i < 8 && (col + i) % 8 != 0);
		}
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

int ve_normal_mode(struct ve_t *self, int key)
{
	switch(key)
	{
	case 'i':
		self->mode = INSERT_MODE;
		break;
	case ':':
		self->mode = PROMPT_MODE;
		ve_prompt_mode(self, key);
		break;
	case 'h':
		ve_next(self, LEFT_KEY);
		break;
	case 'j':
		ve_next(self, DOWN_KEY);
		break;
	case 'k':
		ve_next(self, UP_KEY);
		break;
	case 'l':
		ve_next(self, RIGHT_KEY);
		break;
	case 'w':
		{
			// 0 = [a-zA-Z0-0_]
			// 1 = any other non-whitespace
			// 2 = whitespace
			char res = 0;
			ve_current(self, &res);
			if ('a' <= res && res <= 'z' || 'A' <= res && res <= 'Z' ||
				'0' <= res && res <= '9' || res == '_')
				res = 0;
			else if (res == ' ' || res == '\n' || res == 0)
				res = 2;
			else
				res = 1;

			for (char eof = 0; !eof; ve_eof(self, &eof))
			{
				char cur = 0;
				ve_current(self, &cur);
				if ('a' <= cur && cur <= 'z' || 'A' <= cur && cur <= 'Z' ||
					'0' <= cur && cur <= '9' || cur == '_')
					cur = 0;
				else if (cur == ' ' || cur == '\n' || cur == 0)
					cur = 2;
				else
					cur = 1;
				if (cur == res) ve_next(self, RIGHT_KEY);
				else break;
			}	

			for (char eof = 0; !eof; ve_eof(self, &eof))
			{
				char cur = 0;
				ve_current(self, &cur);
				if (cur == ' ' || cur == '\n')
					ve_next(self, RIGHT_KEY);
				else
					break;
			}
		}
		break;
	case 'W':
		for (char eof = 0; !eof; ve_eof(self, &eof))
		{
			char cur = 0;
			ve_current(self, &cur);
			if (cur == ' ' || cur == '\n')
				break;
			ve_next(self, RIGHT_KEY);
		}
		for (char eof = 0; !eof; ve_eof(self, &eof))
		{
			char cur = 0;
			ve_current(self, &cur);
			if (cur == ' ' || cur == '\n')
				ve_next(self, RIGHT_KEY);
			else break;
		}
	case '$':
		self->ccol = self->lines[self->crow].len;
		break;
	case '0':
		self->ccol = 0;
		break;
	}
	return NO_ERR;
}

int ve_prompt_mode(struct ve_t *self, int key)
{
	switch(key)
	{
	case ENTER_KEY:
		ve_prompt_run(self);

		// reset the prompt
		self->mode = NORMAL_MODE;
		str_free(&self->prompt);
		str_init(&self->prompt);
		break;
	case BACKSPACE_KEY:
		self->prompt.len--;
		if (self->prompt.len == 0)
		{
			self->mode = NORMAL_MODE;
		}
		break;
	default:
		// build the prompt
		if (32 <= key && key <= 126)
			str_appendc(&self->prompt, (char) key);
		break;
	}

	return NO_ERR;
}

int ve_prompt_run(struct ve_t *self)
{
	// reset the message
	str_free(&self->msg);
	str_init(&self->msg);

	// create the prompt
	char *prompt = NULL;
	str_build(&self->prompt, &prompt);
	
	// set the whitespace to nullterminate
	for (int i = 0; i < self->prompt.len; i++)
	{
		if (prompt[i] == ' ')
		{
			prompt[i] = 0;
			break;
		}
	}

	// add a basic hello prompt
	if (strcmp(prompt, ":hello") == 0)
		ve_prompt_run_hello(self);
	else if (strcmp(prompt, ":discard") == 0)
		ve_prompt_run_discard(self);
	else if (strcmp(prompt, ":quit") == 0)
		ve_prompt_run_quit(self);
	else if (strcmp(prompt, ":saveas") == 0)
		ve_prompt_run_saveas(self);
	else if (strcmp(prompt, ":read") == 0)
		ve_prompt_run_read(self);
	else if (strcmp(prompt, ":write") == 0)
		ve_prompt_run_write(self);
	else
	{
		char buffer[80];
		snprintf(buffer, sizeof(buffer), "No such command '%s'", prompt);
		self->is_error = 1;
		str_appends(&self->msg, buffer, strlen(buffer));
	}

	free(prompt);
	return NO_ERR;
}

void ve_prompt_run_hello(struct ve_t *self)
{
	static const char *msg = "Hello, World!";
	str_appends(&self->msg, msg, strlen(msg));
}

void ve_prompt_run_discard(struct ve_t *self)
{
	self->is_running = 0;
}

void ve_prompt_run_quit(struct ve_t *self)
{
	if (self->dirty)
	{
		char buffer[80];
		snprintf(buffer, sizeof(buffer), "File is not saved");
		str_appends(&self->msg, buffer, strlen(buffer));
		self->is_error = 1;
	}
	else
	{
		self->is_running = 0;
	}
}

void ve_prompt_run_saveas(struct ve_t *self)
{
	str_free(&self->filename);
	str_init(&self->filename);

	// get the filename argument
	char *prompt = NULL;
	str_build(&self->prompt, &prompt);
	char buffer[80];
	sscanf(prompt, ":saveas %s", buffer);

	// set the filename state
	str_appends(&self->filename, buffer, strlen(buffer));

	// set the message
	char buffer2[80];
	snprintf(buffer2, sizeof(buffer2), "Filename changed to '%s'", buffer);
	str_appends(&self->msg, buffer2, strlen(buffer2));

	// free the prompt
	free(prompt);
}

void ve_prompt_run_read(struct ve_t *self)
{
	// get the filename argument
	char *prompt = NULL;
	str_build(&self->prompt, &prompt);
	char buffer[80];
	sscanf(prompt, ":read %s", buffer);

	// read the file content
	FILE *fd = fopen(buffer, "r");

	// couldn't open the file
	if (fd == NULL)
	{
		char buffer2[80];
		snprintf(buffer2, sizeof(buffer2), "Couldn't open '%s'", buffer);
		str_appends(&self->msg, buffer2, strlen(buffer2));
		self->is_error = 1;
		free(prompt);
		return;
	}

	// set the file as dirty
	self->dirty = 1;

	// read the contents of the file
	do {
		char ch = fgetc(fd);
		if (feof(fd)) break;

		ve_add(self, ch);
	} while (1);

	fclose(fd);

	// set the message
	char buffer2[80];
	snprintf(buffer2, sizeof(buffer2), "Read '%s'", buffer);
	str_appends(&self->msg, buffer2, strlen(buffer2));

	// free the prompt
	free(prompt);
}

void ve_prompt_run_write(struct ve_t *self)
{
	if (self->filename.len == 0)
	{
		const char *msg = "Filename not specified";
		str_appends(&self->msg, msg, strlen(msg));
		self->is_error = 1;
		return;
	}

	// read the file content
	char *filename = NULL;
	str_build(&self->filename, &filename);
	FILE *fd = fopen(filename, "w");

	// couldn't open the file
	if (fd == NULL)
	{
		char buffer[80];
		snprintf(buffer, sizeof(buffer), "Couldn't open '%s'", filename);
		str_appends(&self->msg, buffer, strlen(buffer));
		self->is_error = 1;
		free(filename);
		return;
	}

	// write the contents of the file
	int bytes = 0;
	for (int i = 0; i < self->sz; i++)
	{
		char *line = NULL;
		str_build(&self->lines[i], &line);
		bytes += fprintf(fd, "%s", line);
		if (i != self->sz - 1)
			bytes += fprintf(fd, "\n");
		free(line);
	}

	// send the message that the file is written
	char buffer[80];
	snprintf(buffer, sizeof(buffer), "'%s' %dL, %dB written", 
		filename, self->sz, bytes);
	str_appends(&self->msg, buffer, strlen(buffer));

	// not dirty anymore
	self->dirty = 0;

	// filename
	free(filename);
}

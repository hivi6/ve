#include <stdlib.h>

#include "util.h"

// ========================================
// string type
// ========================================

int str_init(struct str_t *self)
{
	self->text = NULL;
	self->len = 0;
	self->cap = 0;
	return NO_ERR;
}

int str_free(struct str_t *self)
{
	if (self->text)
		free(self->text);
	return str_init(self);
}

int str_appendc(struct str_t *self, char ch)
{
	if (self->len + 1 >= self->cap)
	{
		// reallocate the array with new capacity
		int old_cap = self->cap;
		int new_cap = (old_cap + 1) * 2;
		char *buffer = (char*) realloc(self->text, new_cap * sizeof(char));
		if (buffer == NULL)
			return MALLOC_ERR;

		self->text = buffer;
		self->cap = new_cap;
	}

	// append the character
	self->text[self->len] = ch;
	self->len++;
	return NO_ERR;
}

int str_appends(struct str_t *self, const char *src, int len)
{
	for (int i = 0; i < len; i++)
	{
		int err = str_appendc(self, src[i]);
		if (err)
			return err;
	}
	return NO_ERR;
}

int str_build(struct str_t *self, char **dest)
{
	// create a new buffer
	char *buffer = (char *) malloc((self->len + 1) * sizeof(char));
	if (buffer == NULL)
		return MALLOC_ERR;

	// copy the content of buffer and set the dest
	for (int i = 0; i < self->len; i++)
		buffer[i] = self->text[i];
	buffer[self->len] = 0;
	*dest = buffer;
	return NO_ERR;
}


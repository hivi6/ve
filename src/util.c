#include <stdlib.h>

#include "util.h"

void ab_init(struct ab_t *ab)
{
	ab->buffer = NULL;
	ab->len = 0;
}

void ab_free(struct ab_t *ab)
{
	free(ab->buffer);
}

void ab_append(struct ab_t *ab, const char *str, int len)
{
	ab->buffer = realloc(ab->buffer, ab->len + len);
	for (int i = 0; i < len; i++)
		ab->buffer[i + ab->len] = str[i];
	ab->len += len;
}

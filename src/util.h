#ifndef UTIL_H
#define UTIL_H

// ========================================
// append_buffer
// ========================================

/**
 * append buffer;
 * used by render functions to generate the final render string
 * and calling the write call once, instead of calling the write call
 * multiple times
 *
 * members
 *	buffer	stores the current buffer
 *	len	length of the buffer
 */
struct ab_t
{
	char *buffer;
	int len;
};

/**
 * intialiaze the append buffer
 *
 * parameters:
 *	ab	self pointer
 */
void ab_init(struct ab_t *ab);

/**
 * free the append buffer
 *
 * parameters:
 *	ab	self pointer
 */
void ab_free(struct ab_t *ab);

/**
 * append the given string to the append buffer
 *
 * parameters:
 *	ab	self pointer
 *	s	string that needs appending
 *	len	length of the string
 */
void ab_append(struct ab_t *ab, const char *s, int len);

#endif // UTIL_H

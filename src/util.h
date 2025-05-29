#ifndef UTIL_H
#define UTIL_H

// ========================================
// error
// ========================================

enum
{
	NO_ERR = 0,
	MALLOC_ERR,
};

// ========================================
// string type
// ========================================

/**
 * string type
 *
 * member:
 *	text	character array to store the string
 *	len	length of the string
 *	cap	capacity of the text array
 */
struct str_t
{
	char *text;
	int len;
	int cap;
};

/**
 * initialize the string pointer
 *
 * params:
 *	self	self pointer
 */
int str_init(struct str_t *self);

/**
 * free the string pointer
 *
 * params:
 *	self	self pointer
 */
int str_free(struct str_t *self);

/**
 * append a single character to the string
 *
 * params:
 *	self	self pointer
 *	ch	character to be appended
 */
int str_appendc(struct str_t *self, char ch);

/**
 * append a null terminated string to the str_t type
 *
 * params:
 *	self	self pointer
 *	src	source string which will be appended
 *	len	length of the string
 */
int str_appends(struct str_t *self, char *src, int len);

/**
 * build a string from the str_t type
 * the user need to free the build string
 *
 * params:
 * 	self	self pointer
 *	dest	where the new string will be built
 */
int str_build(struct str_t *self, char **dest);

#endif // UTIL_H

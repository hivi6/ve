#include <stdio.h>
#include <stdlib.h>

#include "ve.h"

int main()
{
	struct ve_t ve;
	ve_init(&ve);

	ve_next(&ve, '1');
	ve_next(&ve, '2');
	ve_next(&ve, '3');
	ve_next(&ve, '4');
	ve_next(&ve, ENTER_KEY);
	ve_next(&ve, '5');
	ve_next(&ve, '6');
	ve_next(&ve, LEFT_KEY);
	ve_next(&ve, LEFT_KEY);
	ve_next(&ve, LEFT_KEY);
	ve_next(&ve, LEFT_KEY);
	ve_next(&ve, LEFT_KEY);
	ve_next(&ve, '7');
	ve_next(&ve, UP_KEY);
	ve_next(&ve, '8');
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, DOWN_KEY);
	ve_next(&ve, '9');
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, RIGHT_KEY);
	ve_next(&ve, LEFT_KEY);
	ve_next(&ve, DELETE_KEY);

	for (int i = 0; i < ve.sz; i++)
	{
		char *text = NULL;
		str_build(&ve.lines[i], &text);
		printf("[%s]\n", text);
		fflush(stdout);
		free(text);
	}

	ve_free(&ve);

	return 0;
}

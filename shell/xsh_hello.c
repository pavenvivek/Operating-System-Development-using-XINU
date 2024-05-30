/* xsh_hello.c - xsh_hello */

#include <xinu.h>
#include <string.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * xsh_hello - prints a welcome message
 *------------------------------------------------------------------------
 */
shellcmd xsh_hello(int nargs, char *args[]) {

	/* Check argument count */

	/*if (nargs > 2) {
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		fprintf(stderr, "Usage: '%s <string>' \n",
			args[0]);
		return 1;
	}
	else if (nargs < 2) {
		fprintf(stderr, "%s: too few arguments\n", args[0]);
		fprintf(stderr, "Usage: '%s <string>' \n",
			args[0]);
		return 1;
	}*/

	if (nargs != 2) {
		fprintf(stderr, "Syntax: run hello [name]\n");
	}
	else {
		printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);
	}

	return 0;
}

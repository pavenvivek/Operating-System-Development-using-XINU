/* xsh_prodcons.c - xsh_prodcons */

#include <xinu.h>
#include <stdlib.h>
#include <prodcons.h>

int n;

/*------------------------------------------------------------------------
 * xsh_prodcons - create producer and consumer process
 *------------------------------------------------------------------------
 */
shellcmd xsh_prodcons(int nargs, char *args[]) {

	int count = 200;

	/* Check argument count */

	/*if (nargs > 2) {
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		fprintf(stderr, "Usage: '%s <string>' \n",
			args[0]);
		return 1;
	}*/

	if (nargs > 2) {
		fprintf(stderr, "Syntax: run prodcons [counter]\n");
		return 1;
	}
	else if (nargs == 2) {
		count = atoi(args[1]);
	}
	
	can_read = semcreate(0);
	can_write = semcreate(1);
	
	resume(create(producer, 1024, 20, "producer", 1, count));
  	resume(create(consumer, 1024, 20, "consumer", 1, count));
	
	return 0;
}

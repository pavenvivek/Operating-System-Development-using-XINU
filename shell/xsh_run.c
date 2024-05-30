/* xsh_run.c - xsh_run */

#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <prodcons_bb.h>
#include <shprototypes.h>
#include <future_fib.h>
#include <future_prodcons.h>
#include <stream_proc.h>
#include <fstest.h>

int arr_q[5];
sid32 can_read, can_write, mutex, print_sem;
int read_i, write_i;
char *val;
int zero = 0;
int one = 1;
int two = 2;
future_t **fibfut;

void prodcons_bb(int nargs, char *args[]) {
	
	int i = 0, prod_cnt = 0, cons_cnt = 0, prod_itr = 0, cons_itr = 0;

	if (nargs != 5) {
		fprintf(stderr, "Syntax: run prodcons_bb [# of producer processes] [# of consumer processes] [# of iterations the producer runs] [# of iterations the consumer runs]\n");
		
		return;
	}

	prod_cnt = atoi(args[1]);
	cons_cnt = atoi(args[2]);
	prod_itr = atoi(args[3]);
	cons_itr = atoi(args[4]);

	if (prod_cnt == 0 || cons_cnt == 0 || prod_itr == 0 || cons_itr == 0) {
		fprintf(stderr, "Syntax: run prodcons_bb [# of producer processes] [# of consumer processes] [# of iterations the producer runs] [# of iterations the consumer runs]\n");
	
		return;
	}

	if (prod_cnt * prod_itr != cons_cnt * cons_itr) {
		fprintf(stderr, "Iteration Mismatch Error: the number of producer(s) iteration does not match the consumer(s) iteration]\n");
	
		return;
	}

	mutex = semcreate(1);
	can_read = semcreate(0);
	can_write = semcreate(5);

	read_i = 0;
	write_i = 0;

	for (i = 0; i < prod_cnt; i++) {
		resume(create((void *)producer_bb, 4096, 20, "producer_bb", 2, i, prod_itr));
	}

	for (i = 0; i < cons_cnt; i++) {
		resume(create((void *)consumer_bb, 4096, 20, "consumer_bb", 2, i, cons_itr));
	}

}


void future_prodcons(int nargs, char *args[])
{
   	print_sem = semcreate(1);
   	future_t* f_queue;
	char usage[] = "Syntax: run futest [-pc [g ...] [s VALUE ...]] | [-pcq LENGTH [g ...] [s VALUE ...]] | [-f NUMBER] | [--free]\n";

    	// First, try to iterate through the arguments and make sure they are all valid based on the requirements 
    	// (you may assume the argument after "s" there is always a number)
    	int i = 2, j = 0;

	if (i == nargs && (strcmp(args[1], "--free") != 0)) {
		fprintf(stderr, usage);
		return;
	}
	else if (strcmp(args[1], "-pc") == 0 || strcmp(args[1], "-pcq") == 0)
	{
		if (strncmp(args[1], "-pcq", 4) == 0) {

			int length = atoi(args[i]);

			if (length == -1) {
				fprintf(stderr, usage);
				return;
			}
			i++;
			
			if (i == nargs) {
				fprintf(stderr, usage);
				return;
			}
		}
		
		while (i < nargs)
    		{
        		if (strcmp(args[i], "g") == 0) {
				i++;
				continue;
			}
			else if (strcmp(args[i], "s") == 0) {
				j = atoi(args[i+1]);

				if (j == -1) {
					fprintf(stderr, usage);
					return;
				}
				i++;
			}
			else if (strncmp(args[i], "--free", 6) == 0) {
				
				if (i+1 != nargs) {
					fprintf(stderr, usage);
					return;
				}
			}
			else {
				fprintf(stderr, usage);
				return;
			}

			i++; 
    		}
	
	}
	else if (strcmp(args[1], "-f") == 0)
	{	
		j = atoi(args[2]);

		if (j == -1) {
			fprintf(stderr, usage);
			return;
		}

		if (nargs > 3) {
			if (strncmp(args[3], "--free", 6) != 0 || nargs > 4) {
				fprintf(stderr, usage);
				return;
			}
		}
		
	}
	else if (strcmp(args[1], "--free") == 0) {
		if (i > nargs) {
			fprintf(stderr, usage);
			return;
		}
	}
	else {
		fprintf(stderr, usage);
		return;
	}

    	int num_args = i;  // keeping number of args to create the array
    	i = 2;  // reseting the index 
    	val  =  (char *) getmem(num_args); // initializing the array to keep the "s" numbers

        if (strncmp(args[1], "-pc", 3) == 0 || strncmp(args[1], "-pcq", 4) == 0) {
	
		int length = 1;
		if (strncmp(args[1], "-pcq", 4) == 0) {
			length = atoi(args[i]);
			i++;
		}
		f_queue = future_alloc(FUTURE_QUEUE, sizeof(int), length);
    		
		// Iterate again through the arguments and create the following processes based on the passed argument ("g" or "s VALUE")
    		while (i < nargs)
    		{
      			if (strcmp(args[i], "g") == 0){
        			char id[10];
        			sprintf(id, "fcons%d",i);
        			resume(create(future_cons, 2048, 20, id, 1, f_queue));    
      			}
      			if (strcmp(args[i], "s") == 0){
        			i++;
        			uint8 number = atoi(args[i]);
        			val[i] = number;
        			resume(create(future_prod, 2048, 20, "fprod1", 2, f_queue, &val[i]));
        			//sleepms(5);
			}
			if (strncmp(args[i], "--free", 6) == 0) {
				resume(create(future_free_test, 2048, 20, "future_free_test", 2, nargs, args));
			}
			i++;
    		}
    		sleepms(1);
    		future_free(f_queue);
	}
	else if (strncmp(args[1], "-f", 2) == 0) {
        	resume(create(future_fib, 2048, 20, "future_fib", 2, nargs, args));
	
	
		if (strncmp(args[3], "--free", 6) == 0) {
        		resume(create(future_free_test, 2048, 20, "future_free_test", 2, nargs, args));
		}
	}	
	else if (strncmp(args[1], "--free", 6) == 0) {
        	resume(create(future_free_test, 2048, 20, "future_free_test", 2, nargs, args));
	}
	
}


int future_fib(int nargs, char *args[])
{
	int fib = -1, i;

    	fib = atoi(args[2]);

    	if (fib > -1) {
      		int final_fib;
      		int future_flags = FUTURE_SHARED;

 	     	// create the array of future pointers
      		if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1)))
         		 == (future_t **) SYSERR) {
        		printf("getmem failed\n");
        		return(SYSERR);
      		}

      		// get futures for the future array
      		for (i=0; i <= fib; i++) {
        		if((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *) SYSERR) {
          			printf("future_alloc failed\n");
          			return(SYSERR);
        		}
      		}

      		// spawn fib threads and get final value
		for (i=0; i<=fib; i++) {
        		resume(create(ffib, 2048, 20, "ffib", 1, i));
			sleepms(1);
		}

      		future_get(fibfut[fib], (char*) &final_fib);

		for (i=0; i <= fib; i++) {
        		future_free(fibfut[i]);
     		}

      		freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));
      		printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
      		
		return(OK);
	}

	return OK;
}


int future_free_test(int nargs, char *args[])
{
	future_t *f_exclusive;
    	f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
    	printf("future exclsive created\n");
    	future_free(f_exclusive);
    	printf("future exclsive freed\n");

    	future_t *f_shared;
    	f_shared = future_alloc(FUTURE_SHARED, sizeof(int), 1);
    	printf("future shared created\n");
    	future_free(f_shared);
    	printf("future shared freed\n");

    	return OK;
}


/*------------------------------------------------------------------------
 * xsh_run - executes a given function
 *------------------------------------------------------------------------
 */
shellcmd xsh_run(int nargs, char *args[]) {

	/* Check argument count */
	if ((nargs == 1) || (strncmp(args[1], "list", 4) == 0) || 
			((strncmp(args[1], "tscdf_fq", 8) == 0) &&
			 (strncmp(args[1], "hello", 5) != 0) && 
			 (strncmp(args[1], "prodcons_bb", 11) != 0) &&
			 (strncmp(args[1], "prodcons", 8) != 0) &&
			 (strncmp(args[1], "fstest", 6) != 0) &&
			 (strncmp(args[1], "futest", 6) != 0) &&
			 (strncmp(args[1], "tscdf", 5) != 0))) {
      		printf("fstest\n");
      		printf("futest\n");
		printf("hello\n");
      		printf("list\n");
      		printf("prodcons\n");
		printf("prodcons_bb\n");
		printf("tscdf\n");
		printf("tscdf_fq\n");

      		return OK;
    	}

	args++;
	nargs--;


	if(strncmp(args[0], "tscdf_fq", 8) == 0) {
      	/* create a process with the function as an entry point. */
    	  	resume(create((void *)stream_proc_futures, 4096, 20, "stream_proc_futures", 2, nargs, args));
    	}
	else if(strncmp(args[0], "tscdf", 5) == 0) {
      	/* create a process with the function as an entry point. */
    	  	resume(create((void *)stream_proc, 4096, 20, "stream_proc", 2, nargs, args));
	}
	else if(strncmp(args[0], "fstest", 6) == 0) {
      	/* create a process with the function as an entry point. */
    	  	resume(create((void *)fstest, 4096, 20, "fstest", 2, nargs, args));
    	}
	else if(strncmp(args[0], "futest", 6) == 0) {
      	/* create a process with the function as an entry point. */
    	  	resume(create((void *)future_prodcons, 4096, 20, "futest", 2, nargs, args));
    	}
	else if(strncmp(args[0], "hello", 5) == 0) {
      	/* create a process with the function as an entry point. */
    	  	resume(create((void *)xsh_hello, 4096, 20, "hello", 2, nargs, args));
    	}
	else if(strncmp(args[0], "prodcons_bb", 11) == 0) {
      	/* create a process with the function as an entry point. */
    	  	resume(create((void *)prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
    	}
	else if(strncmp(args[0], "prodcons", 8) == 0) {
      	/* create a process with the function as an entry point. */
    	  	resume(create((void *)xsh_prodcons, 4096, 20, "prodcons", 2, nargs, args));
    	}
	
	return 0;
}



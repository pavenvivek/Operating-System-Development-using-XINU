#include <stream_proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tscdf.h"
//#include "tscdf_input.h"

int work_queue_depth;
int num_streams;
int time_window;
int output_time;
uint pcport;

int stream_proc(int nargs, char* args[]) {

	char usage[] = "Usage: run tscdf -s num_streams -w work_queue_depth -t time_window -o output_time\n";
	ulong secs, msecs, time;
  	secs = clktime;
  	msecs = clkticks;
	int i = 0;
	char *ch;
	char c;

    	if (nargs != 9)
    	{
       		printf("%s", usage);
        	return (-1);
    	}
    	else
    	{
        	i = nargs - 1;
        	while (i > 0)
        	{
            		ch = args[i - 1];
            		c = *(++ch);

            		switch (c)
            		{
            			case 's':
                			num_streams = atoi(args[i]);
                			break;

            			case 'w':
                			work_queue_depth = atoi(args[i]);
                			break;

            			case 't':
                			time_window = atoi(args[i]);
                			break;

            			case 'o':
                			output_time = atoi(args[i]);
                			break;

            			default:
                			printf("%s", usage);
                			return (-1);
            		}

            		i -= 2;
        	}
	}


  	if ((pcport = ptcreate(num_streams)) == SYSERR) {
      		printf("ptcreate failed\n");
      		return(-1);
  	}
	
	struct stream st[num_streams];
	struct data_element dte[num_streams][work_queue_depth];

	for (i = 0; i < num_streams; i++) {
		st[i].spaces = semcreate(work_queue_depth);
		st[i].items  = semcreate(0);
		st[i].mutex  = semcreate(1);
		st[i].head   = 0;
		st[i].tail   = 0;
		st[i].queue  = (struct data_element *) &dte[i];

		resume(create((void *)stream_consumer, 4096, 20, "stream_consumer", 2, i, &st[i]));
	}

	//printf ("consumer process creation successfull !\n");
	char *a;
	int id, ts = -1, v = -1;
	int stream_len = n_input; //(sizeof (stream_input1) / sizeof (const char *));

	//printf ("stream_len : %d\n", n_input);

	i = 0;
	while (i < stream_len) {	
		a = (char *)stream_input[i];
    		id = atoi(a);
    		while (*a++ != '\t');
    		ts = atoi(a);
    		while (*a++ != '\t');
    		v = atoi(a);

		//printf ("id:ts:v -> %d:%d:%d\n", id, ts, v);

		wait(st[id].spaces);
		wait(st[id].mutex);

		//printf ("id:ts:v -> %d:%d:%d\n", id, ts, v);
		st[id].queue[st[id].tail].time = ts;
		st[id].queue[st[id].tail].value = v;
		st[id].tail = (st[id].tail + 1) % work_queue_depth;

		signal(st[id].mutex);
		signal(st[id].items);

		i++;
	}

	sleep(0.0000000000005);
	for (i = 0; i < num_streams; i++) {
      		uint32 pm;
      		pm = ptrecv(pcport);
		//wait(st[pm].mutex);
      		printf("process %d exited\n", pm);
		//signal(st[pm].mutex);
  	}

	for (i = 0; i < num_streams; i++) {
		semdelete(st[i].spaces);
		semdelete(st[i].items);
		semdelete(st[i].mutex);
	}

	ptdelete(pcport, 0);

  	time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
  	printf("time in ms: %u\n", time);

	return 0;
}

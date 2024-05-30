#include <stream_proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tscdf.h"

int work_queue_depth;
int num_streams;
int time_window;
int output_time;
uint pcport;

int stream_proc_futures(int nargs, char* args[]) {

	char usage[] = "Usage: run tscdf_fq -s num_streams -w work_queue_depth -t time_window -o output_time\n";
	ulong secs, msecs, time;
  	secs = clktime;
  	msecs = clkticks;
	int i = 0;
	char *ch;
	char c;
	future_t **futures;

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
	
	futures = (future_t**)getmem(num_streams * sizeof(future_t*));

	for (i = 0; i < num_streams; i++) {
		futures[i] = future_alloc(FUTURE_QUEUE, sizeof(struct data_element), work_queue_depth);
		resume(create((void *)stream_consumer_future, 1096, 20, "stream_consumer_future", 2, i, futures[i]));
	}	

	char *a;
	int id, ts = -1, v = -1;
	int stream_len = n_input;
	struct data_element data_el;

	i = 0;
	while (i < stream_len) {	
		a = (char *)stream_input[i];
    		id = atoi(a);
    		while (*a++ != '\t');
		ts = atoi(a);

    		while (*a++ != '\t');
    		v = atoi(a);


		//data_el = (struct data_element *)getmem(sizeof(struct data_element));
		data_el.time = ts;
		data_el.value = v;
		future_set(futures[id], (char *)&data_el);
		//freemem((char*)data_el, sizeof(struct data_element));
		i++;
	}

	sleep(0.0000000000005);
	for (i = 0; i < num_streams; i++) {
      		uint32 pm;
      		pm = ptrecv(pcport);
      		printf("process %d exited\n", pm);
	}

  	ptdelete(pcport, 0);

	for (i = 0; i < num_streams; i++) {
		future_free(futures[i]);
  	}
	freemem((char *)futures, num_streams * sizeof(future_t*));
  	
	time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
  	printf("time in ms: %u\n", time);

	return 0;
}

void stream_consumer_future (int32 id, future_t *f) {
	
	int eos = -1, count = 0;
	struct tscdf *in;
	int32 *qarray;
	char output[100] = {0};
	struct data_element data_el;

	in = tscdf_init(time_window);
	printf("stream_consumer_future id:%d (pid:%d)\n", id, getpid());

	while (eos != 0) {
		//data_el = (struct data_element *)getmem(sizeof(struct data_element));
		future_get(f, (char *)&data_el);

		if (data_el.time == 0) {
			//eos = 0;
			break;
		}

		tscdf_update(in, data_el.time, data_el.value);
		count++;
		
		if (output_time == count) {
			qarray = tscdf_quartiles(in);

    			if(qarray == NULL) {
      				kprintf("tscdf_quartiles returned NULL\n");
      				continue;
    			}

    			sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
    			kprintf("%s\n", output);
    			freemem((char *)qarray, (6*sizeof(int32)));
			count = 0;
		}
		//freemem((char *)data_el, sizeof(struct data_element));
	}

	tscdf_free(in);	
	printf("stream_consumer_future exiting\n");
	ptsend(pcport, getpid());
}


#include <stream_proc.h>
#include "tscdf.h"

void stream_consumer (int32 id, struct stream *st) {
	
	int eos = -1, count = 0;
	struct tscdf *in;
	int32 *qarray;
	char output[100] = {0};

	in = tscdf_init(time_window);
	wait(st->mutex);
	printf("stream_consumer id:%d (pid:%d)\n", id, getpid());
	signal(st->mutex);

	while (eos != 0) {
		wait(st->items);
		wait(st->mutex);
		
		//printf("name : consumer_%d, time : %d, value : %d\n", id, st->queue[st->head].time, st->queue[st->head].value);
		if (st->queue[st->head].time == 0) {
			eos = 0;
			tscdf_free(in);	
			printf("stream_consumer exiting\n");
			//ptsend(pcport, getpid());
			signal(st->mutex);
			signal(st->spaces);
			break;
		}

		tscdf_update(in, st->queue[st->head].time, st->queue[st->head].value);
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
		//else {
		//	count++;
		//}

		//if (st->queue[st->head].time == 0) {
		//	eos = 0;
		//}

		st->head = (st->head + 1) % work_queue_depth;
		
		signal(st->mutex);
		signal(st->spaces);
			
	}
	
	//tscdf_free(in);	
	//wait(st->mutex);
	//printf("stream_consumer exiting\n");
	//signal(st->mutex);
	ptsend(pcport, getpid());
}

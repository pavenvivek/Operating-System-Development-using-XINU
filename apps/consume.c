#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

void consumer(int count) {
	int i = 0;

	for (i = 0; i <= count; i++)
	{
		wait(can_read);
		printf ("consumed : %d\n", n);
		signal(can_write);
	}
}

void consumer_bb(int id, int count) {
	int i = 0;

	for (i = 0; i < count; i++) {
		wait(can_read);
		wait(mutex);
		printf("name : consumer_%d, read : %d\n", id, arr_q[read_i]);
		read_i = (read_i + 1) % 5;
		signal(mutex);
		signal(can_write);
	}
}



#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

void producer(int count) {
  
	int i = 0;

	for (i = 0; i <= count; i++) {
		wait(can_write);
		n = i;
		printf ("produced : %d\n", n);
		signal(can_read);
	}

}

void producer_bb(int id, int count) {
	int i = 0;

	for (i = 0; i < count; i++) {
		wait(can_write);
		wait(mutex);
		arr_q[write_i] = i;
		write_i = (write_i + 1) % 5;
		printf("name : producer_%d, write : %d\n", id, i);
		signal(mutex);
		signal(can_read);
	}
}



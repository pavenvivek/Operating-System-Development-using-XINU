extern int arr_q[5];
extern sid32 can_read, can_write, mutex;
extern int read_i, write_i;

void consumer_bb (int id, int count);
void producer_bb (int id, int count);

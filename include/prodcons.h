#include <xinu.h>

/*Global variable for producer consumer*/
extern int n;
sid32 can_read, can_write;

/*function Prototype*/
void consumer(int count);
void producer(int count);

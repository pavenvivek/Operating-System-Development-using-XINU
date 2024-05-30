/* future.c - future */

#include <xinu.h>
#include <future.h>

/*------------------------------------------------------------------------
 *  future_alloc  -  Allocates memory for future
 *------------------------------------------------------------------------
 */
future_t* future_alloc(
		  future_mode_t mode,	/* Defines the mode of operation for the future */
		  uint		size,	/* size of the data elements in bytes 		*/
		  uint		nelem	/* number of data elements 			*/
		)
{
	intmask		mask;		/* Saved interrupt mask		*/
	future_t*	f;		/* Value to return to caller	*/

	mask = disable();
	
	/* Allocate memory for future */

	f = (future_t *)getmem(sizeof(future_t));
	if (f == (future_t *)SYSERR) {
		printf("future_alloc - insufficient memory");
		restore(mask);
		return (future_t*) SYSERR;
	}

	f->data = (char*)getmem(nelem * size);
	f->size = size;
	f->state = FUTURE_EMPTY;
	f->mode = mode;
	f->pid  = -1;
	f->get_queue = newqueue();
	f->set_queue = newqueue();
	f->max_elems = nelem;
	f->count = 0;
	f->head = 0;
	f->tail = 0;

	restore(mask);
	return f;
}


/*------------------------------------------------------------------------
 *  future_free  -  Frees the allocated memory for future
 *------------------------------------------------------------------------
 */
syscall future_free(
	  future_t* f	/* future to be freed */
	)
{
	intmask		mask;		/* Saved interrupt mask		*/
	int32		retval;
	//pid32		fpid;

	mask = disable();
	
	/*while (nonempty(f->get_queue)) {
		fpid = dequeue(f->get_queue);
		kill(fpid);
	}*/
	delqueue(f->get_queue);

	/*while (nonempty(f->set_queue)) {
		fpid = dequeue(f->set_queue);
		kill(fpid);
	}*/
	delqueue(f->set_queue);
	
	/* Frees the memory allocated for future */
	retval = freemem((char *) f->data, f->max_elems * f->size);
	if (retval == SYSERR) {
		printf("future_free - freeing memory failed");
		restore(mask);
		return SYSERR;
	}

	retval = freemem((char *) f, sizeof(future_t));
	if (retval == SYSERR) {
		printf("future_free - freeing memory failed");
		restore(mask);
		return SYSERR;
	}

	restore(mask);
	return OK;
}


/*------------------------------------------------------------------------
 *  future_get  -  Gets the value of a future
 *------------------------------------------------------------------------
 */
syscall future_get(
	  future_t* f,	/* future to obtain value from */
	  char*	out	/* address to which future value will be copied */
	)
{
	intmask		mask;		/* Saved interrupt mask	*/
	pid32 		curr_id;

	mask = disable();

	if (f->mode == FUTURE_QUEUE) {

		if (f->count == 0) { /* queue is empty */
			curr_id = getpid();
			enqueue(curr_id,f->get_queue);
			suspend(curr_id);
		}
		
		if (f->count > 0) {
			char* headelemptr = f->data + (f->head * f->size);
			memcpy(out, headelemptr, f->size);
			f->head = (f->head + 1) % f->max_elems;
			f->count = f->count - 1;

			if (nonempty(f->set_queue)) {
				resume(dequeue(f->set_queue));
			}

			restore(mask);
			return OK;
		}
	}	
	else if (f->mode == FUTURE_EXCLUSIVE) {
	
		//future_wait_1:
		

		/*if (f->state == FUTURE_WAITING) {
			restore(mask);
			return SYSERR;
		}
		else if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_WAITING;
			f->pid = getpid();
			suspend(f->pid);
			goto future_wait_1;
		}
		else*/ 
		/* removing future waiting for ufu implementation */
		if (f->state == FUTURE_READY) {
			strcpy(out, f->data);
			f->state = FUTURE_EMPTY;
			f->pid = -1;
			restore(mask);
			return OK;
		}
		else {
			restore(mask);
			return SYSERR;
		}	
	}
	else if (f->mode == FUTURE_SHARED) {

		future_wait_2:
		

		if (f->state == FUTURE_WAITING) {
			curr_id = getpid();
			enqueue(curr_id,f->get_queue);
			suspend(curr_id);
			goto future_wait_2;
		}
		else if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_WAITING;
			curr_id = getpid();
			enqueue(curr_id,f->get_queue);
			suspend(curr_id);
			goto future_wait_2;
		}
		else if (f->state == FUTURE_READY) {
			strcpy(out, f->data);
		
			restore(mask);
			return OK;
		}
	}

	restore(mask);
	return OK;
}


/*------------------------------------------------------------------------
 *  future_set  -  Sets the value of a future
 *------------------------------------------------------------------------
 */
syscall future_set(
	  future_t* f,	/* future to set value into */
	  char*	in	/* address from which future value will be copied */
	)
{
	intmask		mask;		/* Saved interrupt mask	*/
	pid32		npid, curr_id;

	mask = disable();


	if (f->mode == FUTURE_QUEUE) {

		if (f->max_elems == f->count) { /* queue is full*/
			curr_id = getpid();
			enqueue(curr_id,f->set_queue);
			suspend(curr_id);
		}
		
		if (f->max_elems > f->count) { /* queue is not full*/
			char* tailelemptr = f->data + (f->tail * f->size);
			memcpy(tailelemptr, in, f->size);
			f->tail = (f->tail + 1) % f->max_elems;
			f->count = f->count + 1;

			if (nonempty(f->get_queue)) {
				resume(dequeue(f->get_queue));
			}

			restore(mask);
			return OK;
		}
	}
	else if (f->mode == FUTURE_EXCLUSIVE) {
		if (f->state == FUTURE_READY) {
			restore(mask);
			return SYSERR;
		}
		else if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_READY;
			strcpy(f->data, in);
			f->pid = -1;
			restore(mask);
			return OK;
		}
		else if (f->state == FUTURE_WAITING) {
			f->state = FUTURE_READY;
			strcpy(f->data, in);
			restore(mask);
			resume(f->pid);
			return OK;
		}	
	}
	else if (f->mode == FUTURE_SHARED) {

		if (f->state == FUTURE_READY) {
			restore(mask);
			return SYSERR;
		}
		else if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_READY;
			strcpy(f->data, in);
			restore(mask);
			return OK;
		}
		else if (f->state == FUTURE_WAITING) {
			f->state = FUTURE_READY;
			strcpy(f->data, in);
		
			while (nonempty(f->get_queue)) {
				npid = dequeue(f->get_queue);
				resume(npid);
			}

			restore(mask);
			return OK;
		}	
	}
	
	restore(mask);
	return OK;
}



#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ringbuf.h"

ringbuf_t * create_ringbuf(unsigned long capacity)
{
	ringbuf_t * rb = malloc(capacity + sizeof(ringbuf_t) );
	if(!rb) {
		printf("malloc(%lu) failed",   capacity + sizeof(ringbuf_t));
		return NULL;
	}

	rb->capacity = capacity;
	rb->length = 0;
	rb->head = 40;
	rb->addr = (char*)rb + sizeof(ringbuf_t);

	MLOGD("capacity:%lu, addr:%p\n", rb->capacity, rb->addr);
	return rb;	
}
int destroy_ringbuf(ringbuf_t * rb)
{
	if(rb){
		free(rb);
	}

	return 0;
}


int write_ringbuf(ringbuf_t * rb, void *data, unsigned long count)
{
	if(count > rb->capacity - rb->length ){
		MLOGD("buf full: capacity %lu, length %lu, count %lu\n",
			rb->capacity, rb->length, count);
		return -1;
	}

	unsigned long tail = (rb->head + rb->length) % rb->capacity;
	if(rb->capacity - tail   >= count ){
		memcpy(rb->addr + tail , data, count);
		MLOGD("copying: head %lu, length %lu, tail %lu count %lu\n", 
			rb->head, rb->length, tail, count);
	}
	else{
		unsigned long first_size = rb->capacity - tail;
		unsigned long second_size = count - first_size;
	
		memcpy(rb->addr+tail, data, first_size);
		memcpy(rb->addr, data + first_size, second_size );

		MLOGD("copying: head %lu, length %lu, tail %lu "
			"count %lu, first %lu, second %lu\n", 
			rb->head, rb->length, tail, count, first_size, second_size);
	}

	rb->length += count;
	MLOGD("write over: head %lu, length %lu\n", rb->head, rb->length);
}

int read_ringbuf(ringbuf_t * rb, void *buf, unsigned long count);


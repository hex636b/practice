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
	if(rb->head > tail || rb->capacity - tail >= count ){
		memcpy(rb->addr + tail , data, count);
		MLOGD("writing   : head %lu, length %lu, tail %lu count %lu\n", 
			rb->head, rb->length, tail, count);
	}
	else{
		unsigned long first_size = rb->capacity - tail;
		unsigned long second_size = count - first_size;
	
		memcpy(rb->addr+tail, data, first_size);
		memcpy(rb->addr, data + first_size, second_size );

		MLOGD("writing   : head %lu, length %lu, tail %lu "
			"count %lu, first %lu, second %lu\n", 
			rb->head, rb->length, tail, count, first_size, second_size);
	}

	rb->length += count;
	MLOGD("write over: head %lu, length %lu, tail %lu\n\n", rb->head, rb->length,
		(rb->head + rb->length) % rb->capacity);
}

int read_ringbuf(ringbuf_t * rb, void *buf, unsigned long count)
{
	if( 0 == rb->length ){
		MLOGD("empty ringbuf\n");
		return -1;
	}

	if(rb->length < count){
		MLOGD("not enough data\n");
		return -1;
	}

	unsigned long tail = (rb->head + rb->length) % rb->capacity;
	if( rb->head < tail || rb->capacity - rb->head >= count ){
		memcpy(buf, rb->addr + rb->head,  count);
		MLOGD("reading  : head %lu, length %lu, tail %lu count %lu\n", 
			rb->head, rb->length, tail, count);
	}
	else{
		unsigned long first_size = rb->capacity - rb->head;
		unsigned long second_size = count - first_size;

		memcpy(buf, rb->addr + rb->head, first_size);
		memcpy(buf+first_size, rb->addr, second_size);	
		MLOGD("reading  : head %lu, length %lu, tail %lu "
			"count %lu, first %lu, second %lu\n", 
			rb->head, rb->length, tail, count, first_size, second_size);
	}
	rb->head = (rb->head + count) % rb->capacity;
	rb->length -= count;
	MLOGD("read over: head %lu, length %lu, tail %lu\n\n", rb->head, rb->length, 
		(rb->head + rb->length) % rb->capacity);

}


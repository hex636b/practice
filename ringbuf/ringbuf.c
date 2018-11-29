#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


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
	rb->head = 0;
	rb->addr = (char*)rb + sizeof(ringbuf_t);

	rb->enable = 1;
	rb->reader_count = 0;
	rb->writer_count = 0;
	pthread_mutex_init(&rb->mutex, NULL);
	pthread_cond_init(&rb->cond, NULL);

	MLOGD("capacity:%lu, addr:%p\n", rb->capacity, rb->addr);
	return rb;	
}
int destroy_ringbuf(ringbuf_t * rb)
{
	if(rb){
		pthread_mutex_lock(&rb->mutex);
		rb->enable = 0;
		pthread_mutex_unlock(&rb->mutex);

		while(rb->reader_count > 0 || rb->writer_count > 0 ){
			MLOGD("reader_count %d, writer_count %d\n", rb->reader_count, rb->writer_count);
			pthread_cond_signal(&rb->cond);
			usleep(1000);
		}
		
		pthread_mutex_destroy(&rb->mutex);
		pthread_cond_destroy(&rb->cond);
		free(rb);
	}

	return 0;
}


int write_ringbuf(ringbuf_t * rb, void *data, unsigned long count)
{
	int ret;
	pthread_mutex_lock(&rb->mutex);
	rb->writer_count++;
	
	if(count > rb->capacity - rb->length ){
		MLOGD("buf full: capacity %lu, length %lu, count %lu\n",
			rb->capacity, rb->length, count);
		rb->writer_count--;
		pthread_mutex_unlock(&rb->mutex);
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
	
	rb->writer_count--;
	pthread_cond_signal(&rb->cond);
	pthread_mutex_unlock(&rb->mutex);
	return 0;
}

int read_ringbuf(ringbuf_t * rb, void *buf, unsigned long count)
{
	pthread_mutex_lock(&rb->mutex);
	rb->reader_count++;

#if 0
	if( 0 == rb->length ){
		MLOGD("empty ringbuf\n");
		pthread_mutex_unlock(&rb->mutex);
		return -1;
	}

	if(rb->length < count){
		MLOGD("not enough data\n");
		pthread_mutex_unlock(&rb->mutex);
		return -1;
	}
#endif
	while(rb->length < count){
		MLOGD("pthread_cond_wait begin\n");
		if(!rb->enable){
			MLOGD("disabled ringbuf, exiting\n");
			rb->reader_count--;
			pthread_mutex_unlock(&rb->mutex);
			return -1;
		}
		pthread_cond_wait(&rb->cond, &rb->mutex);
		MLOGD("pthread_cond_wait out\n");
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
	rb->reader_count--;
	pthread_mutex_unlock(&rb->mutex);
	return 0;

}


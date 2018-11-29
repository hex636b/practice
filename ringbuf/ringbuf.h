#ifndef __SIMPLE_RINGBUF_H__
#define __SIMPLE_RINGBUF_H__
#include <stdio.h>
#include <pthread.h>

#define MLOGD(fmt, arg...) printf("%s:%u  " fmt, __FUNCTION__, __LINE__, ##arg)

typedef struct {
	unsigned long capacity;
	unsigned long head;
	unsigned long length;  //data length
	char * addr;  //start addr

	int enable;
	int reader_count;
	int writer_count;
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
} ringbuf_t;

ringbuf_t * create_ringbuf(unsigned long capacity);
int destroy_ringbuf(ringbuf_t * rb);

int read_ringbuf(ringbuf_t * rb, void *buf, unsigned long count);
int write_ringbuf(ringbuf_t * rb, void *data, unsigned long count);

#endif
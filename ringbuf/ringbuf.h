#ifndef __SIMPLE_RINGBUF_H__
#define __SIMPLE_RINGBUF_H__

#define MLOGD(fmt, arg...) printf("%s:%u >> " fmt, __FUNCTION__, __LINE__, ##arg)

typedef struct {
	unsigned long capacity;
	unsigned long head;
	unsigned long length;  //data length
	char * addr;  //start addr
} ringbuf_t;

ringbuf_t * create_ringbuf(unsigned long capacity);
int destroy_ringbuf(ringbuf_t * rb);

int read_ringbuf(ringbuf_t * rb, void *buf, unsigned long count);
int write_ringbuf(ringbuf_t * rb, void *data, unsigned long count);

#endif
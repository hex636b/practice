#include <stdio.h>
#include <unistd.h>

#include "ringbuf.h"

ringbuf_t * rb;

void * write_thread (void * arg)
{
	int i = 0;
	char data[37] = "123456789";
	
	for(; i<100; i++){
		//usleep(500);
		write_ringbuf(rb, data, sizeof(data));
	}
	
	destroy_ringbuf(rb);
}

void * read_thread (void * arg)
{
	int i = 0;
	int ret;
	char buffer[37] = {};
	while(1){
		ret = read_ringbuf(rb, buffer, sizeof(buffer));
		if(ret < 0){
			MLOGD("exit read thread\n");
			break;
		}
	}

}


int main(int argc, char * argv[])
{
	char buffer[37] = {};
	char data[37] = "123456789";
	rb = create_ringbuf(512);


	pthread_t reader, writer;
	pthread_create(&reader, NULL, read_thread,NULL);
	pthread_create(&writer, NULL, write_thread,NULL);

	pthread_join(reader, NULL);
	pthread_join(writer, NULL);

	return 0;
}

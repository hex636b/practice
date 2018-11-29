#include <stdio.h>
#include "ringbuf.h"



int main(int argc, char * argv[])
{
	char buffer[37] = {};
	char data[37] = "123456789";
	ringbuf_t * rb = create_ringbuf(512);


	int i = 0;
	for(; i<20; i++){
		write_ringbuf(rb, data, sizeof(data));
		read_ringbuf(rb, buffer, sizeof(buffer));
		MLOGD("%s\n\n", buffer);
	}

	destroy_ringbuf(rb);

	return 0;
}

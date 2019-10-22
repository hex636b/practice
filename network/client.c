#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define SERVER_PORT 8400

int main(int argc, char* argv[])
{
	int ret = 0;
	int count = 0;
	int sockfd = -1;
	


	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	inet_aton("127.0.0.1", &addr.sin_addr);

 	while (1) {
		sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_IP);
		if (sockfd < 0) {
			printf("socket %s\n", strerror(errno));
			goto fail;
		}
		printf("connecting ...\n");
		ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
		if (ret < 0) {
			printf("connect fail: %s\n", strerror(errno));
			goto fail;
		}
		count++;
		printf("connect success %d\n", count);

		//close(sockfd);
	}

	

	return 0;

fail:
	if (sockfd != -1) {
		close(sockfd);
	}
	return -1;
}

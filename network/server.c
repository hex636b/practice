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
	int sockfd = -1;
	
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sockfd < 0) {
		printf("socket %s\n", strerror(errno));
		goto fail;
	}

	int value = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
	if(ret) {
		printf("reuse addr %s\n", strerror(errno));
		goto fail;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		printf("bind %s\n", strerror(errno));
		goto fail;
	}

	ret = listen(sockfd, 5);
	if (ret < 0) {
		goto fail;
	}

	int clientfd = -1;

	while(1) {
		memset(&addr, 0, sizeof(addr));
		socklen_t addrlen = sizeof(addr);
		clientfd = accept(sockfd, (struct sockaddr *)&addr, &addrlen);

		printf("client %s:%d connected\n", inet_ntoa((struct in_addr)addr.sin_addr), ntohs(addr.sin_port));
		close(clientfd);
	}

	return 0;

fail:
	if (sockfd != -1) {
		close(sockfd);
	}
	return -1;
}

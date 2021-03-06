#include "common.h"

/*
 * 填充tftp_t结构体，以便发送读写请求
 * 返回:操作码＋文件名＋模式的长度
 */
int fill_request(tftp_t * req, int opcode, char * filename, char * mode )
{
	int len = 0; //统计一共应发送出去多少字节
	char * p = (char*)&(req->be);
	req->opcode = htons(opcode) ; //操作码
	len += 2;

	strcpy(p, filename);   //文件名
	len += strlen(p)+1;

	while( *p++ ); //end of filename, after '\0'
	strcpy( p, mode);	   //模式(netascii或octet)
	len += strlen(p)+1;

	return len;
}

//发送读写请求,有数据发过来即返回0
int send_request( int sockfd,  struct sockaddr_in * dst, tftp_t * req, int reqlen )
{
	int i,ret;
	struct pollfd fds[1] = {sockfd, POLLIN, 0};

	for(i=1; i<=3; i++)
	{
		sendto(sockfd,req, reqlen,0,(SAP)dst, sizeof(*dst));
		ret = poll(fds, 1, TIMEOUT*1000); 
		if( ret > 0 && fds[0].revents & POLLIN )
		{
			break;       //有数据收到
		}
		else if( 3==i )
		{
			fprintf(stderr, "time out\n");
			return -1;
		}			
	}

	return 0; 
}


int main(int argc, char ** argv )
{
	int i,ret;
	int fd,sockfd;
	tftp_t req = {0};  //用于发送读写请求
	tftp_t ack = {0};
	int reqlen = 0;
	struct sockaddr_in server = {0}; //服务器端地址
	struct sockaddr_in sender = {0};
	int addrlen = sizeof(sender);

	if( argc < 4 )   //参数不够
	{
		fprintf(stderr,"usage: tftp <server IP> <get|put> <file name>\n");
		return -1;
	}
	
	//设置服务器地址
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = inet_addr(argv[1]);
	if( INADDR_NONE == server.sin_addr.s_addr)
	{
		fprintf(stderr,"IP addr invalid");
		return -1;
	}
	//创建套接字	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if( sockfd < 0 )
	{
		perror("socket");
		return -1;
	}
	
	if( 0 == strcmp(argv[2], "get") )  //下载文件，覆盖现有文件
	{
		fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644); 
		if( fd < 0 )
		{
			perror("open");
			goto ERROR_OUT;
		}
		reqlen = fill_request( &req, RRQ, argv[3], "octet" );

		ret = send_request(sockfd, &server, &req, reqlen); //发送请求
		if( ret < 0 )
			goto ERROR_OUT;
		
		ret = recv_file(sockfd, fd, &server, argv[2]); //接收文件
		if( 0 != ret )
			goto ERROR_OUT;
	}
	else if( 0 == strcmp(argv[2], "put") ) //上传文件
	{
		fd = open(argv[3], O_RDONLY );
		if( fd < 0 )
		{
			perror("open");
			goto ERROR_OUT;
		}
		reqlen=fill_request( &req,WRQ,argv[3],"octet");

		for(i=1;i<=3; i++ )
		{
			ret = send_request(sockfd, &server, &req, reqlen); //发送请求
			if( ret < 0 )
				goto ERROR_OUT;

			addrlen = sizeof(sender);
			recvfrom(sockfd, &ack, sizeof(ack),0,(SAP)&sender,&addrlen); 

			if( sender.sin_addr.s_addr == server.sin_addr.s_addr )
			{
				if( ntohs(ack.opcode) == ACK  &&  0 == ntohs(ack.be.block) )
					break;   //收到0号确认，服务器启用新端口，新地址放在sender

				if( ntohs(ack.opcode) == ERROR )
				{
					fprintf(stderr,"ERROR code %d: %s\n",
								 ntohs(ack.be.error), ack.data);
					goto ERROR_OUT;
				}
			}
			if( 3 == i )
				goto ERROR_OUT;
		}

		ret = send_file(sockfd, fd, &sender, argv[2]);  
		if( 0 != ret )
			goto ERROR_OUT;
	}
	else
	{
		fprintf(stderr,"usage: client <server IP> <get|put> <filename>\n");
		return -1;	
	}

	close(fd);
	close(sockfd);
	return 0;

ERROR_OUT:
	if( sockfd > 0 )
		close(sockfd);
	if ( fd > 0 )
		close(fd);
	return -1;
} //end of main()


#include "common.h"

//比较两个地址的IP，端口号是否相同，相同返回0，不相同返回－1
int addrcmp(const struct sockaddr_in * sa1, const struct sockaddr_in * sa2)
{
	if( sa1->sin_port == sa2->sin_port && sa1->sin_addr.s_addr == sa2->sin_addr.s_addr )
		return 0;
	return -1;
}

//发送确认，确认号为blknum
int send_ack( int sockfd, struct sockaddr_in * dst, int blknum )
{
	ack_t ack;

	ack.opcode = htons(ACK);
	ack.blknum = htons(blknum);

	if( 4 != sendto(sockfd, &ack,4,0,(SAP)dst,sizeof(*dst)) )
	{
		perror("sendto");
		return -1;
	}
	return 0; 
}

int recv_file( int sockfd, int fd, struct sockaddr_in * peer, const char* mode)
{
	int ret;	
	int next = 1;  //下一个应接收的块编号

	tftp_t buf = {0};
	struct pollfd fds[1] = {0} ;
	struct sockaddr_in sender = {0};
	int addrlen = sizeof(sender);

	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	while(1)
	{
		ret = poll(fds, 1, TIMEOUT*1000); 
		if( 1 != ret )
		{
			fprintf(stderr,"time out\n");
			return -1;
		}
		
		//收数据
		addrlen = sizeof(sender);
		ret = recvfrom(sockfd,&buf, sizeof(buf), 0, (SAP)&sender, &addrlen);
		if( 1 == next )
		{
			if( sender.sin_addr.s_addr == peer->sin_addr.s_addr )
				memcpy(peer, &sender, sizeof(sender));  //对客户端来说，存储服务器新地址
			else
				continue; //不相关IP发来的数据
		}
		else if( 0 != addrcmp(peer, &sender) )
		{
			continue; //不相关地址发来的数据
		}		
		
		//收到期望的数据块	
		if( ntohs(buf.opcode) == DATA  )
		{
			if( ntohs(buf.be.block) <  next )
			{
				send_ack(sockfd, &sender, ntohs(buf.be.block) );
				continue;
			}
			else if( ntohs(buf.be.block) == next )
			{			
				send_ack(sockfd, &sender, next);  //发送确认		
				write(fd, buf.data, ret-4 );      //写入磁盘
				next++;

				if( ret-4 < BLKSIZE )   //传输完成
					return 0;
			}			
		}
		else if( ntohs(buf.opcode) == ERROR )  //收到错误信息
		{
			fprintf(stderr,"ERROR code %d: %s\n", ntohs(buf.be.error), buf.data);
			return -1;
		}
	}

	return 0;
}//end of recv_file

int send_file( int sockfd, int fd, struct sockaddr_in * peer, const char* mode)
{
	int next = 1;  //下一个应发送的块编号
	int i, ret;
	
	tftp_t buf = {0};
	tftp_t ack = {0};
	struct pollfd fds[1] = {sockfd, POLLIN} ;
	struct sockaddr_in sender = {0};
	int addrlen = sizeof(sender); 
	
	while(1)
	{
		buf.opcode = htons(DATA);
		buf.be.block = htons(next);
		ret = read(fd,buf.data, BLKSIZE); //从本地读取文件,ret记录读到的字节数
	
		//发送数据块并等待确认,至多尝试5次
		for( i=1; i<=5; i++)
		{
			sendto(sockfd, &buf, 4+ret, 0, (SAP)peer, sizeof(*peer)); //发出数据块

			if( poll(fds,1,TIMEOUT*1000) > 0 && fds[0].revents & POLLIN)
			{
				addrlen = sizeof(sender);
				recvfrom(sockfd, &ack, sizeof(ack),0,(SAP)&sender,&addrlen); //接收块确认

				if( 0 == addrcmp(peer, &sender) )
				{
					if( ACK == ntohs(ack.opcode) && next == ntohs(ack.be.block) )
						break;       //收到块确认

					if( ERROR == ntohs(ack.opcode) )
					{
						fprintf(stderr,"ERROR CODE %d: %s\n",
								ntohs(buf.be.error),ack.data);
						return -1;
					}
				}
			}

			if( 5 == i )
			{
				fprintf(stderr,"send file time out\n");
				return -1;
			}
		}

		next++; //下一个数据块

		if( ret < BLKSIZE )   //传输完成
			return 0;
	}

	return 0;
}

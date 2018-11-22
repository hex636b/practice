#include "common.h"

static error_t errors[8] = {
	{ERROR, EUNDEF,       "not defined error code"},
	{ERROR, ENOTFOUND, "file not found"},
	{ERROR, EACCESS,      "access violation"},
	{ERROR, ENOSPACE,    "disk full or allocation exceeded"},
	{ERROR, EBADOP,       "illegal TFTP operation"},
	{ERROR, EBADID,        "unknown transfer ID"},
	{ERROR, EEXISTS,       "file already exists"},
	{ERROR, ENOUSER,      "no such user"}
};

void sig_handler(int sig)
{
	if( SIGCHLD==sig )
	{
		while( waitpid(-1,NULL,0) > 0 ); //收到SIGCHLD信号，回收子进程
	}
}

//初始化错误列表
void init_errors()
{
	int i = 0;
	for( i=0; i<=7; i++ )
	{
		errors[i].opcode  = htons(ERROR);
		errors[i].errcode = htons(i);
	}	
}

//初始化服务器自已的地址，创建SOCKET，返回socket描述符
int init_socket( )
{
	int sockfd;
	int on = 1;
	int ret;
	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = INADDR_ANY;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if( sockfd < 0 )
	{
		perror("socket");
		return -1;
	}

	ret = setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR,&on,sizeof(on));
	if( ret < 0 )
	{
		perror("setsockopt");
		close(sockfd);
		return -1;
	}
	ret = bind(sockfd,(SAP)&server,sizeof(server));
	if( ret < 0 )
	{
		perror("bind");
		close(sockfd);
		return -1;
	}
	return sockfd;  //返回socket描述符
}

//初始化服务器端，错误列表，...
void init_server()
{
	init_errors();
	signal(SIGCHLD,sig_handler); //注册信号处理函数
	chdir("/var/lib/tftpboot");  //切换主目录
}


//发送错误信息,错误号为err_code， 务必先使errors数组初始化
int send_error(int sockfd, struct sockaddr_in * dst,int err_code)
{
	int length = 0;
	
	if( err_code < 0 || err_code > 7 )
		return -1;

	length = 4 + strlen(errors[err_code].info) + 1;  //含字符串尾部'\0'
	
	if ( length != sendto(sockfd, &errors[err_code],length,0,(SAP)dst,sizeof(*dst) ) )
	{
		perror("sendto");
		return -1;
	}
	return 0;
}

//创建新SOCKET，打开/创建文件，具体文件传输由recv_file()和send_file()实现
int service( tftp_t * req, struct sockaddr_in * peer)
{
	int sockfd;
	int fd;
	int ret;

	int opcode = ntohs(req->opcode);
	char * filename = (char*)&(req->be);
	char * mode = NULL;

	mode = filename;
	while(*mode++); //暂时统一按octet模式
	
	//用新的SOCKET与客户端传递数据
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if( sockfd < 0 )
	{
		perror("socket");
		return -1;
	}	
	
	if(RRQ==opcode)   //客户读请求，下载
	{
		if( access(filename, F_OK))  //文件不存在
		{
			send_error(sockfd,peer,ENOTFOUND);
			goto ERROR_OUT;
		}

		fd = open(filename,O_RDONLY); //读方式打开
		if( fd < 0 )
		{
			perror("open");
			send_error(sockfd, peer, EACCESS);
			goto ERROR_OUT;
		}

		ret = send_file(sockfd, fd, peer, mode); //开始发送数据
		if( ret < 0 )
			goto ERROR_OUT;
	}
	else if( WRQ == opcode )  //客户端写请求，上传
	{
		fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0644); 
		if( fd < 0 )
		{
			perror("open");
			send_error(sockfd, peer, EEXISTS); //文件已存在
			goto ERROR_OUT;
		}
		
		send_ack(sockfd, peer, 0);    //发送0号确认
		
		ret = recv_file(sockfd, fd, peer, mode); //接收文件数据
		if( ret < 0 )
			goto ERROR_OUT;
	}
	else    //非法操作码
	{
		send_error(sockfd,peer,EBADOP);
		goto ERROR_OUT;
	}

	close(fd);	
	close(sockfd);
	return 0;

ERROR_OUT:
	if( fd > 0 )
		close(fd);
	if( sockfd > 0 )
		close(sockfd);
	return -1;
}

int main(int argc, char ** argv )
{
	int ret;
	int sockfd;

	pid_t pid;
	tftp_t req = {0};	

	struct sockaddr_in peer = {0}; //客户端地址
	int addrlen = sizeof(peer);
	

	init_server();

	sockfd = init_socket();
	if( sockfd < 0 )
		return -1;

	while(1)
	{
		//读取客户请求
		addrlen = sizeof(peer);
		ret = recvfrom(sockfd,&req,sizeof(req),0,(SAP)&peer, &addrlen);
		if( ret <= 0 )
			continue;

		pid = fork();
		if( 0==pid)
		{
			close(sockfd);
			service(&req, &peer); //交子进程处理
			return 0;
		}
	}	

	close(sockfd);
	return 0;

} //end of main()


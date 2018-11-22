#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>

#include "http_server.h"

static const status_info_s status_table[] = {
	{200, "OK"},
	{400, "Bad Request"},
	{404, "Not Found"},
	{500, "Internal Server Error"},
	//...	
};

static const char * method_table[] = {
	"GET", "POST", "PUT", "DELETE","OPTIONS","HEAD"
};


static CGI_INFO_S cgi_table[24] = {};


static server_conf_s server_conf;

static int listen_fd = -1;

int is_valid_method(const char * method)
{
	int i = 0;
	for(i=0; i < ARRAY_SIZE(method_table); i++) {
		if( 0 == strcmp(method, method_table[i]) ){
			return 1;
		}
	}
	return 0;
}

const char * get_status_phrase(unsigned int status)
{
	int i = 0;
	for(i = 0; i < ARRAY_SIZE(status_table); i++){
		if(status == status_table[i].status_code){
			return status_table[i].reason_phrase;
		}
	}
	return "NULL";
}

int http_server_init(server_conf_s * conf)
{
	int ret = 0;

	memset(cgi_table, 0, sizeof(cgi_table));
	memcpy(&server_conf, conf, sizeof(server_conf_s));
	
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		perror("socket");
		return -1;
	}
	
	int reuse = 1;
	ret = setsockopt(listen_fd,SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(ret < 0 ){
		perror("setsockopt");
		close(listen_fd);
	}
	
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(conf->listen_port);
	if(bind(listen_fd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr_in))<0)
	{
		perror("bind");
		close(listen_fd);
		return -1;
	}
	
	if( listen(listen_fd, 5) < 0)
	{
		perror("listen");
		close(listen_fd);
		return -1;
	}

	return 0;
}

int http_server_stop()
{
	if(listen_fd >= 0){
		close(listen_fd);
	}
	return 0;
}

int parse_request(int client_fd, char * recv_buf, int buf_size, http_request_s * request)
{
	char * temp = recv_buf;
	char * method = recv_buf;
	char * url = NULL; 
	char * version = NULL;
	memset(request, 0, sizeof(http_request_s));

	//parse start line(method, url, version)
	while( ' ' != *temp){
		temp++;
	}
	*temp = '\0';
	if( ! is_valid_method(method)){
		return -1;
	}
	strncpy(request->start_line.method, method, sizeof(request->start_line.method)-1);
	
	url = ++temp;
	while( ' ' != *temp){
		temp++;
	}
	*temp = '\0';	
	strncpy(request->start_line.url, url, sizeof(request->start_line.url)-1);

	version = ++temp;
	while( '\r' != *temp && '\n' != *temp){
		temp++;
	}
	*temp = '\0';
	strncpy(request->start_line.version, version, sizeof(request->start_line.version)-1);

	temp++;
	if( '\n' == *temp){
		temp++;
	}
	
	//parse header
	
	return 0;
}


int send_response(int fd, unsigned int status, char * content)
{
	char send_buf[512] = {};
	snprintf(send_buf, sizeof(send_buf),
		"HTTP/1.1 %u %s\r\n"
		"Content-Length: %lu\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"%s\r\n\r\n",
		status, get_status_phrase(status), strlen(content)+4, content);
	
	write(fd, send_buf, strlen(send_buf)+1);
	printf("\n%s\n", send_buf);
	return 0;
}

// possible url: /cgi-bin/sum.cgi?a=3&b=5
//               /cgi-bin/sum.cgi?&a=3&b=5
//               /cgi-bin/sum.cgi?&-a=3&-b=5

int parse_params_from_url(char * url, http_param_s * params)
{
	int ret = 0;
	char * temp = url;
	char *name_start, *name_end;
	char *value_start, *value_end;
	
	memset(params, 0, sizeof(http_param_s));

	//find first '?'
	while( '?' != *temp && '\0' != *temp ){
		temp++;
	}
	if( '\0' == *temp ){
		return -1;  //not found
	}
	//now temp points to '?'

	
	temp++; //temp points to '\0'  or '&' or name_start
	while('\0' != *temp )
	{
		name_start = NULL;
		name_end = NULL;
		value_start = NULL;
		value_end = NULL;
		
		name_start = temp;
		if('&' == *name_start){
			name_start++;
		}
		if('-' == *name_start){
			name_start++;
		}
		temp = name_start;
		
		while( '=' != *temp && '\0' != *temp ){
			temp++;
		}
		if('\0' == *temp){
			return 0;
		}
		//now temp points to '='
		
		name_end = temp-1;


		temp++;
		value_start = temp;
		while( '&' != *temp && '\0' != *temp ){
			temp++;
		}
		//now temp points to '&'  or '\0'

		value_end = temp-1;

		strncpy(params->pair[params->count].name, name_start, (size_t)((size_t)name_end - (size_t)name_start + 1));
		strncpy(params->pair[params->count].value, value_start, (size_t)((size_t)value_end - (size_t)value_start + 1));
		params->count++;

	}

	return 0;
}

int get_param_value(const char * name, http_param_s * params, char * value, size_t size)
{
	int i = 0;
	for(i=0; i < params->count; i++)
	{
		if(0 == strcmp(name, params->pair[i].name) ){
			strncpy(value, params->pair[i].value, size);
		}
	}

	return -1;
}


// INPUT url:  "/cgi-bin/sum.cgi?arg1=1111&arg2=8888"
// RETURN: "sum.cgi"
int parse_cgi_name(const char * url, char * cgi_name, size_t size)
{
	const char * start = url + strlen(server_conf.cgi_dir);
	const char * temp = start;

	memset(cgi_name, 0, size);

	while( '?' != *temp && '\0' != *temp ){
		temp++;
	}

	if(temp-start+1 > size-1){
		return -1;  //too long
	}

	strncpy( cgi_name, start, temp-start);  //exclude '?'
	return 0;
}

int handle_cgi(int client_fd, http_request_s * request)
{
	int i = 0;
	int ret = 0;
	char cgi_name[32] = {};
	ret = parse_cgi_name(request->start_line.url, cgi_name, sizeof(cgi_name) );
	if(ret){
		printf("parse_cgi_name fail\n");
		return -1;
	}
	http_param_s params = {};
	parse_params_from_url(request->start_line.url, &params);

	printf("cgi:%s\n", cgi_name);
	char body[256] = {};
	for(i=0; i<ARRAY_SIZE(cgi_table); i++)
	{
		printf("%s: %s\n",cgi_name, cgi_table[i].cgi_name);
		if( 0 == strcmp(cgi_name, cgi_table[i].cgi_name)){
			ret = cgi_table[i].cgi_func( &params, body, sizeof(body));
			if(ret){
				printf("cgi callback return error\n");
				return -1;
			}
			send_response(client_fd, 200, body);
			return 0;
		}
	}
	printf("cgi not found\n");
	return -1;
}



void * handle_connection(void * arg)
{
	int ret = 0;
	int client_fd = arg;
	char recv_buf[1024];
	int recv_count = 0;
	int recv_bytes = 0;
	memset(recv_buf,'\0',sizeof(recv_buf));

	//receive HTTP start line and headers
	do{
		recv_bytes = read(client_fd, &recv_buf[recv_count], sizeof(recv_buf) - recv_count); 
		recv_count += recv_bytes;
	}while(recv_count < sizeof(recv_buf)  && NULL == strstr(recv_buf, "\r\n\r\n"));
	
	if( NULL == strstr(recv_buf, "\r\n\r\n") ){
		//HTTP head too long!
		goto err;
	}
	
	printf("\n%s\n",recv_buf);
	http_request_s request = {};
	ret = parse_request(client_fd, recv_buf,  recv_count, &request);
	if(ret){
		goto err;
	}

	if( 0 == strcmp("GET", request.start_line.method)
		&& 0 == strncmp(request.start_line.url, server_conf.cgi_dir, strlen(server_conf.cgi_dir) ) ){

		ret = handle_cgi(client_fd,  &request);
		if(ret){
			printf("handle_cgi fail\n");
			goto err;
		}
		return NULL;
	}

	//only support cgi now
	
err:
	send_response(client_fd, 400, "ERROR");	
	close(client_fd);
	return NULL;
}

void http_server_run()
{
	struct sockaddr_in client_addr;
	 for(;;)
	{
		socklen_t len = 0;
		int client_fd = accept(listen_fd,(struct sockaddr*)&client_addr,&len);
		if(client_fd < 0){
			perror("accept");
			return;
		}
		
		pthread_attr_t attr = {0};
		pthread_t thread_id;
		pthread_attr_init (&attr); 
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread_id, &attr, handle_connection, (void*)client_fd);
	}

}

int http_server_register_cgi(CGI_INFO_S * cgi_info)
{
	int i = 0;
	for(i=0; i<ARRAY_SIZE(cgi_table); i++){
		if( 0 == strlen(cgi_table[i].cgi_name) ){
			memcpy( &cgi_table[i], cgi_info, sizeof(CGI_INFO_S));
			return 0;
		}
	}

	return -1;
}


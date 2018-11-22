#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "http_server.h"

int  sum_cgi_callback(http_param_s * params, char * http_body, size_t size)
{
	char buffer1[32] = {};
	char buffer2[32] = {};

	int ret = 0;
	get_param_value("arg1", params, buffer1, sizeof(buffer1));
	get_param_value("arg2", params, buffer2, sizeof(buffer2));
	
	if(0==strlen(buffer1) || 0 == strlen(buffer2)){
		return -1;
	}
	int sum = atoi(buffer1) + atoi(buffer2);
	snprintf(http_body, size, "%d", sum);
	return 0;
}

int main(int argc, char * argv[])
{
	server_conf_s server_conf = {};
	server_conf.listen_port = 8080;
	strncpy(server_conf.cgi_dir, "/cgi-bin/hisnet/", sizeof(server_conf.cgi_dir)-1);

	http_server_init( &server_conf );

	CGI_INFO_S cgi_sum = {};
	strcpy(cgi_sum.cgi_name, "sum.cgi");
	cgi_sum.cgi_func = sum_cgi_callback;
	http_server_register_cgi(&cgi_sum);

	http_server_run();

	return 0;
}

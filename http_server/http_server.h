#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__



#define logd(fmt, args...) fprintf(stdout,"%s() %u: " fmt,__FUNCTION__, __LINE__, ##args)
#define loge(fmt, args...) fprintf(stderr,"%s() %u: " fmt,__FUNCTION__, __LINE__, ##args)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define MIN(a,b)  (a>b ? : b : a)

typedef struct {
	char method[16];
	char url[256];
	char version[16];
}http_start_line_s;

typedef struct {
	//name-value collections
}http_header_s;

typedef struct{
	http_start_line_s start_line;
	http_header_s     header;
	//body info...
}http_request_s;

typedef struct {
	unsigned int status_code;
	const char * reason_phrase;
}status_info_s;

typedef struct{
	char name[16];
	char value[32];
}name_value_pair_s;

typedef struct{
	unsigned int count;  //valid items
	name_value_pair_s pair[16];
}http_param_s;


typedef struct 
{
	unsigned short listen_port;
	char cgi_dir[32];
}server_conf_s;

//give params to callback function, get http_body to response.
typedef (*CGI_CALLBACK)(http_param_s * params, char * http_body, size_t size);
typedef struct {
	char cgi_name[32];
	CGI_CALLBACK cgi_func;
}CGI_INFO_S;


int http_server_init(server_conf_s * conf);
int http_server_register_cgi(CGI_INFO_S * cgi_info);
int get_param_value(const char * name, http_param_s * params, char * value, size_t size);

int http_server_stop();

void http_server_run();



#endif


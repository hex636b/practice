#ifndef _TFTP_H_
#define _TFTP_H_

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#define SAP struct sockaddr *

#define SERVER_PORT   69
#define BLKSIZE             512
#define TIMEOUT           5

#pragma pack(1)

//通用数据结构，也可用来发送最初的读写请求，涉及到接收数据的，均使用此类型
typedef struct
{
	short opcode;  //operate code 

	union 
	{
		short block; // block number
		short error ; //error code 
	} __attribute__ ((__packed__)) be;    //block number  or  error code

	char data[BLKSIZE] ;

} tftp_t ;  

typedef struct 
{
	short opcode ;
	short errcode ;
	char  info[60] ;
}error_t;

typedef struct
{
	short opcode;
	short blknum;
}ack_t;

#pragma pack() 

/*
 * operate code
 */
#define	RRQ    01			/* read request */
#define	WRQ    02			/* write request */
#define	DATA   03			/* data packet */
#define	ACK    04			/* acknowledgement */
#define	ERROR  05			/* error code */

/*
 * Error codes.
 */
#define	EUNDEF		0		/* not defined */
#define	ENOTFOUND	1		/* file not found */
#define	EACCESS     2		/* access violation */
#define	ENOSPACE	3		/* disk full or allocation exceeded */
#define	EBADOP		4		/* illegal TFTP operation */
#define	EBADID		5		/* unknown transfer ID */
#define	EEXISTS		6		/* file already exists */
#define	ENOUSER     7		/* no such user */

int send_ack( int sockfd, struct sockaddr_in * dst, int blknum );

int addrcmp(const struct sockaddr_in * sa1, const struct sockaddr_in * sa2);

int send_file( int sockfd, int fd, struct sockaddr_in * peer, const char* mode);

int recv_file( int sockfd, int fd, struct sockaddr_in * peer, const char* mode);



#endif

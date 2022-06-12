#define _GNU_SOURCE 1		/* 使用“POLLRDHUP”宏需定义该宏 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <poll.h>

int main (int argc, char * argv[])
{ 
	int c_fd,pid; 
	int ret = 0;
	struct sockaddr_in s_addr;
	socklen_t addr_len;
	char buf[1024]; 
	ssize_t size;
	fd_set s_fds;
	struct pollfd p_fds[5];  	/* 最大监听5个句柄 */
	int max_fd; 				/* 监听文件描述符中最大的文件号 */

	if(argc != 3)
	{
		printf("format error!\n");
		printf("usage: client <address> <port>\n");
		exit(1);
	}
	
	/* 创建socket */
	c_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(c_fd < 0)
	{
		perror("socket create failed");
		return -1;
	} 
 
	/* 服务器端地址 */ 
	s_addr.sin_family 	= AF_INET;
	s_addr.sin_port 	= htons(atoi(argv[2]));
	if(!inet_aton(argv[1], (struct in_addr *) &s_addr.sin_addr.s_addr))
	{
		perror("invalid ip addr");
		exit(1); 
	}

	/* 连接服务器*/
	addr_len = sizeof(s_addr);
	ret = connect(c_fd, (struct sockaddr*)&s_addr, addr_len); 
	if(ret < 0)
	{
		perror("connect server failed");
		exit(1);  
	} 

	/* poll 监听参数 */
	for(ret = 0;ret < 5;ret++)
	{
		p_fds[ret].fd = -1;
	}
	p_fds[0].fd = STDIN_FILENO;
	p_fds[0].events = POLLIN;
	p_fds[0].revents = 0;
	p_fds[1].fd = c_fd;
	p_fds[1].events = POLLIN | POLLRDHUP | POLLPRI;
	p_fds[1].revents = 0;
	max_fd = 1;

	for(;;)
	{
		ret = poll(p_fds,max_fd+1,-1);			/* 监听5个句柄；-1表示阻塞，不超时 */

		if(ret < 0)
		{
			perror("poll error");
            break;
		}
		else if(ret == 0)
		{
			continue;
		}

		if((p_fds[0].revents&POLLIN) == POLLIN && p_fds[0].fd == STDIN_FILENO) 
		{
			fflush(stdout);
			memset(buf, 0, sizeof(buf)); 
			size = read(STDIN_FILENO, buf, sizeof(buf) - 1); 	
			if(size > 0) 
			{
				buf[size-1] = '\0'; 
			} 
			else
			{
				perror("read stdin error"); 
				break;
			}
			if(!strncmp(buf, "quit", 4))
			{
				printf("close the connect!\n");
				break;
			}
			if(buf[0] == '\0')
			{
				printf("please enter message to send:\n"); 
				continue;
			}
			size = write(c_fd, buf, strlen(buf)); 
			if(size <= 0)
			{
				printf("message'%s' send failed!errno code is %d,errno message is '%s'\n",buf, errno, strerror(errno));
				break;
			}
			printf("please enter message to send:\n"); 
		}

		if((p_fds[1].revents&POLLRDHUP) == POLLRDHUP && p_fds[1].fd == c_fd) 
		{/* disconnect */
			printf("server disconnect!\n");
			break;
		}
		
		if((p_fds[1].revents&POLLIN) == POLLIN && p_fds[1].fd == c_fd) 
		{
			memset(buf, 0, sizeof(buf)); 
			size = read(c_fd, buf, sizeof(buf) - 1);
			if(size > 0)
			{
				printf("message recv %dByte: \n%s\n",size,buf);
			}
			else if(size < 0)
			{
				printf("recv failed!errno code is %d,errno message is '%s'\n",errno, strerror(errno));
				break;
			}
			else
			{
				printf("server disconnect!\n");
				break;
			}	
		}
		
	}
	
	close(c_fd); 

	return 0; 
}

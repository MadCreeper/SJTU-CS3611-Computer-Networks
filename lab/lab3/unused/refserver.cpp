#define _GNU_SOURCE 1		/* 使用“POLLRDHUP”宏需定义该宏 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <resolv.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <poll.h>

int main (int argc, char * argv[])
{
	int s_fd = 0, c_fd = 0, pid; 
	socklen_t addr_len;
	struct sockaddr_in s_addr, c_addr;
	char buf[1024];
	ssize_t size = 0;
	struct pollfd p_fds[5];  	/* 最大监听5个句柄 */
	int max_fd;
	int ret = 0;
	
	if(argc != 3)
	{
		printf("format error!\n");
		printf("usage: server <address> <port>\n");
		exit(1);
	}
	
	/* 服务器端地址 */ 
	s_addr.sin_family = AF_INET;
	s_addr.sin_port   = htons(atoi(argv[2]));
	if(!inet_aton(argv[1], (struct in_addr *) &s_addr.sin_addr.s_addr))
	{
		perror("invalid ip addr:");
		exit(1); 
	}
	
	/* 创建socket */
	if((s_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("socket create failed:"); 
		exit(1); 
	} 

	/* 端口重用，调用close(socket)一般不会立即关闭socket，而经历“TIME_WAIT”的过程 */
	int reuse = 0x01;
	if(setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int)) < 0)
	{
		perror("setsockopt error");
		close(s_fd);
		exit(1); 
	}
	
	/* 绑定地址 */
	if(bind(s_fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) < 0)
	{
		perror("bind error");
		close(s_fd);
		exit(1); 
	}
	
	/* 监听socket */
	if(listen(s_fd, 5) < 0)
	{
		perror("listen error");
		close(s_fd);
		exit(1); 
	}
	addr_len = sizeof(struct sockaddr);

	/* poll 监听参数 */
	for(ret = 0;ret < 5;ret++)
	{
		p_fds[ret].fd = -1;
	}
	p_fds[0].fd = STDIN_FILENO;
	p_fds[0].events = POLLIN;
	p_fds[0].revents = 0;
	p_fds[1].fd = s_fd;
	p_fds[1].events = POLLIN;
	p_fds[1].revents = 0;
	max_fd = 1;
	
	for(;;)
	{
		ret = poll(p_fds,max_fd+1,2000);			/* 监听5个句柄；-1表示阻塞，不超时 */

		if(ret < 0)
		{
			perror("poll error");
            break;
		}
		else if(ret == 0)
		{
			continue;
		}
		
		if(((p_fds[1].revents&POLLIN) == POLLIN) && (p_fds[1].fd == s_fd)) 
		{
			if(c_fd)
			{
				continue;
			}
			printf("waiting client connecting\r\n");
			c_fd = accept(s_fd, (struct sockaddr*)&c_addr, (socklen_t *)&addr_len);
			
			if(c_fd < 0)
			{
				perror("accept error");
				close(s_fd);
				break; 
			}
			else
			{
				printf("connected with ip: %s and port: %d\n", inet_ntop(AF_INET,&c_addr.sin_addr, buf, 1024), ntohs(c_addr.sin_port));
			}
			/* 将客户端加入监听集合中 */
			p_fds[2].fd = c_fd;
			p_fds[2].events = POLLIN | POLLRDHUP | POLLPRI;
			p_fds[2].revents = 0;
			max_fd = 2;
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

		if((p_fds[2].revents&POLLRDHUP) == POLLRDHUP && p_fds[2].fd == c_fd) 
		{/* disconnect */
			printf("client disconnect!\n");
			break;
		}
		
		if((p_fds[2].revents&POLLIN) == POLLIN && p_fds[2].fd == c_fd) 
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
				printf("client disconnect!\n");
				break;
			}	
		}
	}

	close(s_fd); 
	close(c_fd);

	return 0;
}
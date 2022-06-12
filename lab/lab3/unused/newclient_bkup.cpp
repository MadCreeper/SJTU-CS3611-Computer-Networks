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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>

#define PORT "2333"
#define MAXDATASIZE 100
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void parse_recv_ip(char buf[], char ret[]){ // find ip in "tell" command
    int i;
    for(i = 5; buf[i] != ' '; ++i){
        ret[i - 5] = buf[i];
    }
    ret[i - 5] = '\0';
}
int main (int argc, char * argv[])
{
    if(argc != 2)
	{
		printf("format error!\n");
		printf("usage: server <address> \n");
		exit(1);
	}
    for(;;){
	int s_fd = 0, c_fd = 0;
	socklen_t addr_len;
	struct sockaddr_in s_addr, c_addr;
	char buf[1024];
	ssize_t size = 0;
	struct pollfd p_fds[5];  	/* 最大监听5个句柄 */
	int max_fd;
	int ret = 0;
	
	
	
	/* 服务器端地址 */ 
	s_addr.sin_family = AF_INET;
	s_addr.sin_port   = htons(atoi(PORT));

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
            if(!strncmp(buf, "tell", 4)){
                int sockfd, numbytes;  
                char recv_ip[INET6_ADDRSTRLEN];
                struct addrinfo hints, *servinfo, *p;
                int rv;
                char s[INET6_ADDRSTRLEN];
                memset(&hints, 0, sizeof hints);

                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;

                parse_recv_ip(buf, recv_ip);
                std::cout << "recv ip:" << recv_ip << std::endl;
                if ((rv = getaddrinfo(recv_ip, PORT, &hints, &servinfo)) != 0) {
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                    return 1;
                }

                // loop through all the results and connect to the first we can
                for(p = servinfo; p != NULL; p = p->ai_next) {
                    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                            p->ai_protocol)) == -1) {
                        perror("client: socket");
                        continue;
                    }

                    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                        close(sockfd);
                        perror("client: connect");
                        continue;
                    }

                    break;
                }

                if (p == NULL) {
                    fprintf(stderr, "client: failed to connect\n");
                    return 2;
                }

                inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                        s, sizeof s);
                printf("client: connecting to %s\n", s);

                freeaddrinfo(servinfo); // all done with this structure

                if ((numbytes = send(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }


                close(sockfd);

                continue;

			}

		}
		
		if((p_fds[2].revents&POLLIN) == POLLIN && p_fds[2].fd == c_fd) 
		{
			memset(buf, 0, sizeof(buf)); 
			size = recv(c_fd, buf, sizeof(buf) - 1, 0);
			if(size > 0)
			{
				printf("message recv %dByte: \n%s\n",size,buf);
                break;
			}
			else if(size < 0)
			{
				printf("recv failed!errno code is %d,errno message is '%s'\n",errno, strerror(errno));
				break;
			}
		
		}
	}
    close(s_fd); 
	close(c_fd);
    }
	

	return 0;
}
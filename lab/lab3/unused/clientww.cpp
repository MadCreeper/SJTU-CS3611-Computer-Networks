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
#include <map>

#define PORT "2333"
#define MAXDATASIZE 100
#define BUFSIZE 1024
#define TCOMM "To"
#define MAXPOLLFDS 5

std::map <std::string, std::string> NAME_IP_MAP{
	{"h1","10.0.0.1"},
	{"h2","10.0.0.2"},
    {"h3","10.0.0.3"},
    {"h4","10.0.0.4"}
};

std::map <std::string, std::string> IP_NAME_MAP{
	{"10.0.0.1","h1"},
	{"10.0.0.2","h2"},
    {"10.0.0.3","h3"},
    {"10.0.0.4","h4"},
};

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


bool parse_recv_ip(char buf[], char ret_ip[], char msg[]){ // find ip in "tell" command
    int i = strlen(TCOMM) + 1;
	std::string hname = "";
    for(; buf[i] != ' ' && buf[i] != '\0'; ++i){
        hname += buf[i];
    }
	++i;

    int j = 0;
    if(NAME_IP_MAP.find(hname) == NAME_IP_MAP.end()){
        return false;
    }
	std::string h_ip = NAME_IP_MAP[hname];

	for(int k = 0; k < h_ip.length(); ++k){
		ret_ip[j++] = h_ip[k];
	}
	ret_ip[j]='\0';
	//std::cout << ret_ip << std::endl;
	j = 0;
	for (; buf[i] != '\0'; ++i){
		msg[j++] = buf[i];
	}
    msg[j] = '\0';
	//std::cout << msg << std::endl;
    return true;
}

int main (int argc, char * argv[])
{
    
    for(;;){
        int s_fd = 0, c_fd = 0;
        socklen_t addr_len;
        struct sockaddr_in s_addr, c_addr;
        char buf[BUFSIZE];
        ssize_t size = 0;
        struct pollfd poll_fds[MAXPOLLFDS];  	
        int max_fd;
        int ret = 0;
        
        // set server address
        s_addr.sin_family = AF_INET;
        s_addr.sin_port   = htons(atoi(PORT));

        if(!inet_aton("0.0.0.0", (struct in_addr *) &s_addr.sin_addr.s_addr)){
            perror("invalid ip addr:");
            exit(1); 
        }

        
        // create socket
        if((s_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket create failed:"); 
            exit(1); 
        } 
        

        // reuse socket
        int reuse = 0x01;
        if(setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int)) < 0) {
            perror("setsockopt error");
            close(s_fd);
            exit(1); 
        }
        
        // bind addr
        if(bind(s_fd, (struct sockaddr*)&s_addr, sizeof(s_addr)) < 0) {
            perror("bind error");
            close(s_fd);
            exit(1); 
        }
        
        // listen
        if(listen(s_fd, MAXPOLLFDS) < 0) {
            perror("listen error");
            close(s_fd);
            exit(1); 
        }
        addr_len = sizeof(struct sockaddr);
        
        
        for(ret = 0;ret < MAXPOLLFDS;ret++) {
            poll_fds[ret].fd = -1;
        }
        poll_fds[0].fd = STDIN_FILENO;
        poll_fds[0].events = POLLIN;
        poll_fds[0].revents = 0;
        poll_fds[1].fd = s_fd;
        poll_fds[1].events = POLLIN;
        poll_fds[1].revents = 0;
        max_fd = 1;
        for(;;)
        {
            ret = poll(poll_fds,max_fd+1,2000);			

            if(ret < 0) {
                perror("poll error");
                break;
            }
            else if(ret == 0){
                continue;
            }
            
            if(((poll_fds[1].revents&POLLIN) == POLLIN) && (poll_fds[1].fd == s_fd)){
                if(c_fd)
                {
                    continue;
                }
                std::cout << "------------------" << std::endl;
                c_fd = accept(s_fd, (struct sockaddr*)&c_addr, (socklen_t *)&addr_len);
                
                if(c_fd < 0){
                    perror("accept error");
                    close(s_fd);
                    break; 
                }
                else{
                    std::string incoming_ip( inet_ntop(AF_INET,&c_addr.sin_addr, buf, 1024));
                    int incoming_port = ntohs(c_addr.sin_port);

                    std::cout << "From " << IP_NAME_MAP[incoming_ip] 
                    << '('  << incoming_ip << ':' << incoming_port << ')' << std::endl;
                }
               
                poll_fds[2].fd = c_fd;
                poll_fds[2].events = POLLIN | POLLRDHUP | POLLPRI;
                poll_fds[2].revents = 0;
                max_fd = 2;
            }

            if((poll_fds[0].revents&POLLIN) == POLLIN && poll_fds[0].fd == STDIN_FILENO) {
                fflush(stdout);
                memset(buf, 0, sizeof(buf)); 
                size = read(STDIN_FILENO, buf, sizeof(buf) - 1); 	
                if(size > 0){
                    buf[size-1] = '\0'; 
                } 
                else {
                    perror("read stdin error"); 
                    break;
                }
                if(!strncmp(buf, "exit", 4)) {
                    break;
                }
                if(!strncmp(buf, TCOMM, strlen(TCOMM))){
                    int sockfd, numbytes;  
                    char recv_ip[INET6_ADDRSTRLEN];
                    struct addrinfo hints, *servinfo, *p;
                    int rv;
                    char s[INET6_ADDRSTRLEN];
                    char msg[BUFSIZE];
                    memset(&hints, 0, sizeof hints);

                    hints.ai_family = AF_INET;
                    hints.ai_socktype = SOCK_STREAM;

                    if(! parse_recv_ip(buf, recv_ip, msg)){
                        std::cout << "Unknown host name!" << std::endl;
                        exit(1);
                    }
                    //std::cout << "recv ip:" << recv_ip << std::endl;
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
                    // printf("client: connecting to %s\n", s);

                    freeaddrinfo(servinfo); // all done with this structure

                    if ((numbytes = send(sockfd, msg, sizeof(msg), 0)) == -1) {
                        perror("recv");
                        exit(1);
                    }

                    close(sockfd);
                    continue;
                }
            }
            
            if((poll_fds[2].revents&POLLIN) == POLLIN && poll_fds[2].fd == c_fd) 
            {
                memset(buf, 0, sizeof(buf)); 
                size = recv(c_fd, buf, sizeof(buf) - 1, 0);
                if(size >= 0)
                {
                    std::cout << "> " << buf << std::endl;
                    break;
                }
                else {
                    std::cout << "Error receiving message!" << std::endl;
                    break;
                }
            
            }
        }
        close(s_fd); 
        close(c_fd);
    }
	

	return 0;
}
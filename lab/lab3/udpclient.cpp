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
#include <ifaddrs.h>
#include <string>
#include <iostream>
#include <map>

#define PORT "2334"
#define MAXDATASIZE 100
#define BUFSIZE 1024
#define TCOMM "To"
#define MAXPOLLFDS 5
#define BROADCASTIP "10.255.255.255"
#define FILTERLOOPBACK true
#define _GNU_SOURCE 1
const char SUBNET[] = "10.0.0.";

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

std::string get_self_ip(){
    //Adapted from https://stackoverflow.com/questions/212528/how-can-i-get-the-ip-address-of-a-linux-machine
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { 
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if(strncmp(SUBNET, addressBuffer, strlen(SUBNET)) == 0){
                return std::string(addressBuffer);
            }
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    
    return std::string("");
}


int main (int argc, char * argv[])
{
    std::string self_ip = get_self_ip();
    std::cout << "Staring UDP chat. Self IP is: " << self_ip << std::endl;
    if(FILTERLOOPBACK){
        std::cout << "Self message will not be shown" << std::endl;
    }
    for(;;){
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
        struct sockaddr_storage their_addr;
        char buf[BUFSIZE];
        socklen_t addr_len;
        char s[INET6_ADDRSTRLEN];
        struct pollfd poll_fds[MAXPOLLFDS];
        int max_fd;
        int ret = 0;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if ((rv = getaddrinfo(BROADCASTIP, PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop through all the results and bind to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("listener: socket");
                continue;
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("listener: bind");
                continue;
            }

            break;
        }

        if (p == NULL) {
            fprintf(stderr, "listener: failed to bind socket\n");
            return 2;
        }

        freeaddrinfo(servinfo);

        
        
        for(ret = 0;ret < MAXPOLLFDS;ret++) {
            poll_fds[ret].fd = -1;
        }
        poll_fds[0].fd = STDIN_FILENO;
        poll_fds[0].events = POLLIN;
        poll_fds[0].revents = 0;
        poll_fds[1].fd = sockfd;
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
            
            if(((poll_fds[1].revents&POLLIN) == POLLIN) && (poll_fds[1].fd == sockfd)){
                addr_len = sizeof their_addr;
                if ((numbytes = recvfrom(sockfd, buf, BUFSIZE-1 , 0,
                    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
                }

                std::string incoming_ip(inet_ntop(their_addr.ss_family, 
                                        get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
                
                buf[numbytes] = '\0';
                if (!(FILTERLOOPBACK && incoming_ip == self_ip)){
                    std::cout << IP_NAME_MAP[incoming_ip] 
                          << "(" << incoming_ip << "):\n> " << buf << std::endl;
                }
                
                
           
            }

            if((poll_fds[0].revents&POLLIN) == POLLIN && poll_fds[0].fd == STDIN_FILENO) {
                fflush(stdout);
                memset(buf, 0, sizeof(buf)); 
                ssize_t size = read(STDIN_FILENO, buf, sizeof(buf) - 1); 	
                if(size > 0){
                    buf[size-1] = '\0'; 
                }

                int send_fd;
                struct addrinfo s_hints, *s_servinfo, *p2;
                int sendaddr;
                socklen_t addr_len;
                char s[INET6_ADDRSTRLEN];

                memset(&s_hints, 0, sizeof s_hints);
                s_hints.ai_family = AF_INET; // set to AF_INET to use IPv4
                s_hints.ai_socktype = SOCK_DGRAM;
            
                if ((sendaddr = getaddrinfo(BROADCASTIP, PORT, &s_hints, &s_servinfo)) != 0) {
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(sendaddr));
                    return 1;
                }

                // loop through all the results and bind to the first we can
                for(p2 = s_servinfo; p2 != NULL; p2 = p2->ai_next) {
                    if ((send_fd = socket(p2->ai_family, p2->ai_socktype, p2->ai_protocol)) == -1) {
                        perror("socket");
                        continue;
                    }
                    break;
                }

                if(p2 == NULL){
                    perror("socket create failed");
                    return 2;
                }

                const int yes = 1;
                if (setsockopt(send_fd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &yes,
                    sizeof(int)) == -1) {
                    perror("setsockopt");
                    exit(1);
                }

                //std::cout << "Got stdin input:"  << buf << std::endl;

                if ((sendto(send_fd, buf, strlen(buf), 0,p2->ai_addr, p2->ai_addrlen)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }

                freeaddrinfo(s_servinfo);

            }
                        
        }
        close(sockfd); 
        
    }
	

	return 0;
}
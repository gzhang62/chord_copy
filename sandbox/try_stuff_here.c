#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <netdb.h>

#define MY_PORT 1000

#include <ifaddrs.h>
bool match_any_interface() {
    struct ifaddrs *if_addrs, *host;
    
    if (getifaddrs(&if_addrs) == -1){
        perror("getifaddrs");
        return false;
    }

    int s;
    for (host = if_addrs; host != NULL; host = host->ifa_next){
        if (host->ifa_addr == NULL){
            continue;
        }
        //char *ip = malloc();
        //getaddrinfo(ip,));
        struct sockaddr_in *sa = (struct sockaddr_in *) host->ifa_addr; 
        printf("%s\n",inet_ntoa(sa->sin_addr));
        //s = getnameinfo(host->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    }
    return true;
}

/*
int main()
{
    char myIP[16];
    unsigned int myPort;
    struct sockaddr_in server_addr, my_addr;
    int sockfd;

    // Connect to server
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Can't open stream socket.");
        exit(-1);
    }

    // Get my ip address and port
    bzero(&my_addr, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    getsockname(sockfd, (struct sockaddr *) &my_addr, &len);
    inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
    myPort = ntohs(my_addr.sin_port);

    printf("Local ip address: %s\n", myIP);
    printf("Local port : %u\n", myPort);

    match_any_interface();

    return 0;
} */

int main() {
    char myIP[16];
    unsigned int myPort;
    struct sockaddr_in server_addr, my_addr;
    int server_fd;

    char hostbuffer[256];
    struct hostent *host_entry;
    // To retrieve hostname
    int hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    printf("%s\n",hostbuffer);
    /*host_entry = gethostbyname(hostbuffer);
    char ** addr_list = host_entry->h_addr_list;
    printf("%d\n",host_entry->h_length);
    for(int i = 0; i < host_entry->h_length; i++) {
        if(addr_list[i]) {
            printf("%d: %s\n",i,inet_ntoa(*((struct in_addr*) addr_list[i])));
        }
    }*/
    
    /**
    struct addrinfo hints, *res, *result;
    memset(&hints, 0, sizeof    (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int err = getaddrinfo(hostname, NULL, &hints, &result);
    result->ai_addr->sa_data
    */

	// set up chord node socket
	if((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("Failed to create socket");
	}

	// zero out addr struct
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = (unsigned short) MY_PORT;		

	// bind socket to address
	if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("Failed to bind socket to address");
	}

    bzero(&my_addr, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    getsockname(server_fd, (struct sockaddr *) &my_addr, &len);
    inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
    myPort = ntohs(my_addr.sin_port);

    printf("Local ip address: %s\n", myIP);
    printf("Local port : %u\n", myPort);

}
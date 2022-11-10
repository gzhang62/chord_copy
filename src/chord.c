#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"

#define MAX_CLIENTS 1024

void printKey(uint64_t key) {
	printf("%" PRIu64, key);
}

int main(int argc, char *argv[]) {
	int num_clients = 0;
	int clients[MAX_CLIENTS]; // keep track of fds, if fd is present, fds[i] = 1 else fds[i] = 0
	/* Server Socket Variables */
	int server_fd, len, server_port;
	struct sockaddr_in server_addr;

	/* Select variables */
	int maxfd = 0;
	fd_set readset;
	struct timeval timeout;

	/* Timestamps for periodic update variables*/
	struct timespec last_check_predecessor;
	struct timespec last_fix_fingers;
	struct timespec last_stabilize;
	/* timeout values in seconds */
	int cpp;
	int ffp;
	int sp;
	
	// set up chord node socket
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		exit_error("Failed to create socket");
	}

	// zero out addr struct
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);			// get from args

	// bind socket to address
	if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		exit_error("Failed to bind socket to address");
	}

	// set socket to listen
	if(listen(server_fd, MAX_CLIENTS) < 0) {
		exit_error("Failed to listen on socket");
	}

	timeout.tv_sec = 1;
	FD_ZERO(&readset);	// zero out readset
	FD_SET(server_fd, &readset);	// add server_fd
	FD_SET(0, &readset);	// add stdin

	for(;;) {
		int ret = select(maxfd + 1, &readset, NULL, NULL, &timeout);

		if(ret == -1) {
			// error
		} else if(ret) {
			if(FD_ISSET(server_fd, &readset)) {
				// handle a new connection
				handle_connection(server_fd);
			}	

			if(FD_ISSET(0, &readset)) {
				// handle stdin command
				read_process_input(server_fd);
			} 

			for(int i = 0; i < num_clients; i++) {
				if(FD_ISSET(clients[i], &readset)) {
					// process client
					read_process_node(clients[i]);
				}
			}
		}

		// check timeout
		if(check_time(&last_stabilize, sp)) {
			// stabilize()
		}

		if(check_time(&last_check_predecessor, cpp)) {
			// check_predecessor()
		}

		if(check_time(&last_fix_fingers, ffp)) {
			// fix_fingers()
		}
	}

	return 0; 
}

int check_time(struct timespec *last_time, int timeout) {
	struct timespec curr_time;
	clock_gettime(CLOCK_REALTIME, &curr_time);
	
	if(curr_time.tv_sec - last_time->tv_sec <= timeout) {
		return 1;
	} else {
		return 0;
	}
}

void exit_error(char * error_message) {
	perror(error_message);
	exit(EXIT_FAILURE);
}


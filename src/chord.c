#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"

#define MAX_CLIENTS 1024
struct sha1sum_ctx *ctx;


/* Timestamps for periodic update variables*/
struct timespec last_check_predecessor;
struct timespec last_fix_fingers;
struct timespec last_stabilize;

void printKey(uint64_t key) {
	printf("%" PRIu64, key);
}

int main(int argc, char *argv[]) {
	int num_clients = 0;
	int clients[MAX_CLIENTS]; // keep track of fds, if fd is present, fds[i] = 1 else fds[i] = 0
	int server_fd;
	/* Select variables */
	int maxfd = 0;
	fd_set readset;
	struct timeval timeout;
	// Parse 
	struct chord_arguments chord_args = chord_parseopt(argc, argv);
	// uint8_t num_successors = chord_args.num_successors;
	struct sockaddr_in my_address = chord_args.my_address;
	// struct sockaddr_in join_address = chord_args.join_address;
	/* timeout values in seconds */
	int cpp = chord_args.check_predecessor_period;
	int ffp = chord_args.fix_fingers_period;
	int sp = chord_args.stablize_period;

	server_fd = setup_server(ntohs(my_address.sin_port));
	FD_ZERO(&readset);	// zero out readset
	FD_SET(server_fd, &readset);	// add server_fd
	FD_SET(0, &readset);	// add stdin
	
	int cpret = clock_gettime(CLOCK_REALTIME, &last_check_predecessor);
	int ffret = clock_gettime(CLOCK_REALTIME, &last_fix_fingers);
	int spret = clock_gettime(CLOCK_REALTIME, &last_stabilize);


	for(;;) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int ret = select(maxfd + 1, &readset, NULL, NULL, &timeout);

		if(ret == -1) {
			// error
		} else if(ret) {
			if(FD_ISSET(server_fd, &readset)) {
				// handle a new connection
				int client_socket = handle_connection(server_fd);

				clients[num_clients] = client_socket;
				num_clients ++;

				FD_SET(client_socket, &readset);
			}	

			if(FD_ISSET(0, &readset)) {
				// handle stdin command
				// read_process_input(server_fd);
			} 

			for(int i = 0; i < num_clients; i++) {
				if(FD_ISSET(clients[i], &readset)) {
					// process client
					// read_process_node(clients[i]);
				}
			}
			check_periodic(cpp, ffp, sp);
		} else {
			check_periodic(cpp, ffp, sp);
		}
	}

	ctx = sha1sum_create(NULL, 0);
	return 0;
}

/**
 * Read value from node and process accordingly.
 * @author Adam 
 * @param sd socket descriptor for node
 * @return 0 if success, -1 otherwise
 */
int read_process_node(int sd) {
	return -1;
}

/**
 * Read value from standard in and process accordingly.
 * @author Adam 
 * @param fd file descriptor for standard in
 * @return 0 if success, -1 otherwise
 */
int read_process_input(int fd) {
	int ret;
	// Take input, parse into command
	size_t size = 1;
	char *input = (char *) malloc(size), *command, *key;
	int bytes_read = getline(&input, &size, fd); // Assuming fd is stdin
	
	if(ret < 0) { // read error
		perror("Input read error encountered\n"); ret = -1;
	} else if(size <= 2) {
	    perror("No command provided\n"); ret = -1;
	} else {
    	input[size-2] = '\0'; //remove newline
    	command = strtok_r(input, " ",&key);
	    //input before first space is in command, input after is in key
		//(key is empty string if there is no space)
	    //printf("COMMAND: [%s], KEY: [%s]", command, key);

		// Determine if it's valid command / what command it is	    
		if(strcmp(command, "Lookup") == 0) { // lookup
			if(strcmp(key,"") == 0) {
				perror("No key passed into Lookup command"); ret = -1;
			} else {
				ret = lookup(key);
			}
		} else if(strcmp(command, "PrintState") == 0) { // print state
			if(strcmp(key,"") != 0) {
				perror("Extra parameter passed in to PrintState"); ret = -1;
			} else {
				ret = print_state();
			}
		} else { // wrong command
			perror("Wrong command entered\n"); ret = -1;
		}
	}
	free(input);
	return ret;
}

int lookup(char *key) {
	//printf("Lookup not implemented\n");
	//Get hash of key
	uint64_t key_id = get_hash(key); 
	Node *result = find_successor(key_id);
	uint64_t node_id = get_node_hash(result);
	
	// Print results
	struct in_addr ip_addr;
	ip_addr.s_addr = result->address;

	printf("< %s",key);
	printf(" %" PRIu64, node_id); //I don't understand how this works
	printf(" %s %u\n", inet_ntoa(ip_addr), result->address); 

	free(result);
	return 0; 
}

/**
 * Test whether timout seconds have elapsed, and a periodic function should be run
 * @author Gary
 * @param last_time timestamp of when periodic function was last run
 * @param timeout timeout in seconds 
 * @return 1 if timeout time has elapsed, 0 otherwise
 */
int check_time(struct timespec *last_time, int timeout) {
	struct timespec curr_time;
	int cret = clock_gettime(CLOCK_REALTIME, &curr_time);
	
	if(curr_time.tv_sec - last_time->tv_sec >= timeout) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * print out an error message and shut down the program
 * @author Gary
 * @param error_message print out an error message and shut down the program
 * @return void
 */
void exit_error(char * error_message) {
	perror(error_message);
	exit(EXIT_FAILURE);
}

/**
 * setup server socket
 * @author Gary
 * @param  
 * @return new server socket
 * @todo currently terminates everything on failure, more refinement may be needed
 */
int setup_server(int server_port) {
	int server_fd;
	struct sockaddr_in server_addr;
	// set up chord node socket
	if((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		exit_error("Failed to create socket");
	}

	// zero out addr struct
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);		

	// bind socket to address
	if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		exit_error("Failed to bind socket to address");
	}

	// set socket to listen
	if(listen(server_fd, MAX_CLIENTS) < 0) {
		exit_error("Failed to listen on socket");
	}

	return server_fd;
}

/**
 * handle a new node asking for a connection
 * @author Gary
 * @param sd server socket
 * @param num_clients number of clients
 * @param clients array of client fds already connected
 * @return new client socket
 */
int handle_connection(int sd) {
	struct sockaddr_in client_address;
	socklen_t len = sizeof(client_address);
	int client_fd = accept(sd, (struct sockaddr *)&client_address, &len);
	return client_fd;
}

Node *closest_preceding_node(uint64_t id) {
	for(int i = NUM_BYTES_IDENTIFIER-1; i >= 0; i--) {
		uint32_t finger_key = finger[i].key;
		if(finger_key > n.key && finger_key < id) {
			return &finger[i];
		}
	}

	return &n;
}

Node *find_successor(uint64_t id) {
	// might have to loop through all successors
	Node *immediate_sucessor = &successors[0];
	if(immediate_sucessor->key >= id && id > n.key) {
		return immediate_sucessor;
	} else {
		Node *n_prime = closest_preceding_node(id);
		FindSuccessorRequest fsreq = FIND_SUCCESSOR_REQUEST__INIT;
		FindSuccessorResponse *fsresp;
		void *send_buf;
		void *recv_buf;
		unsigned send_len;
		unsigned recv_len;
		bool message_received = false;
		int new_sock;

		// create new temp socket
		if((new_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
			exit_error("Failed to create socket");
		}	

		// set request id
		fsreq.key = id;
		// get packed len
		send_len = find_successor_request__get_packed_size(&fsreq);
		// allocate a buffer and pack message
		send_buf = malloc(send_len);
		find_successor_request__pack(&fsreq, send_buf);

		// send message to n_prime's address
		if(connect(new_sock, (struct sockaddr *) n_prime->address, sizeof(n_prime->address))) {
			exit_error("Failed to connect to n_prime in find successor");
		}
		if(send(new_sock, send_buf, send_len, 0)) {
			exit_error("Failed to send find successor request");
		}

		recv_len = find_successor_response__get_packed_size(&fsresp); // unsure if this is how to do it
		recv_buf = malloc(recv_len);
		if(recv(new_sock, recv_buf, recv_len, MSG_PEEK)) {
			exit_error("Failed to receive find successor response");
		}
		fsresp = find_successor_response__unpack(NULL, recv_len, recv_buf);
		
		close(new_sock);
		free(send_buf);
		return fsresp->node;
	}
}

void check_periodic(int cpp, int ffp, int sp) {
	// check timeout
	if(check_time(&last_stabilize, sp)) {
		// stabilize()
		printf("Stabilize\n");
		fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &last_stabilize); // should go into function above
	}

	if(check_time(&last_check_predecessor, cpp)) {
		// check_predecessor()
		printf("Check Predecessor\n");
		fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &last_check_predecessor); // should go into function above
	}

	if(check_time(&last_fix_fingers, ffp)) {
		// fix_fingers()
		printf("Fix fingers\n");
		fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &last_fix_fingers); // should go into function above
	}
}


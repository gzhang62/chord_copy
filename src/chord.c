#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"

struct sha1sum_ctx *ctx;

Callback callback_array[RAND_MAX];
AddressTable *address_table;


// Num successors
uint8_t num_successors;

void printKey(uint64_t key) {
	printf("%" PRIu64, key);
}

int main(int argc, char *argv[]) {
	address_table = NULL;

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
	num_successors = chord_args.num_successors;
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
	UNUSED(cpret);
	UNUSED(ffret);
	UNUSED(spret);

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
	printf("> "); // indicate we're waiting for user input
	return 0;
}

/**
 * Read value from node and process accordingly.
 * @author Adam 
 * @param sd socket descriptor for node
 * @return 0 if success, -1 otherwise
 */
int read_process_node(int sd)	{
	int return_value = -1;

	ChordMessage *message = receive_message(sd);
	// Decide what to do based on message case
	switch(message->msg_case) {
		case CHORD_MESSAGE__MSG_NOTIFY_REQUEST: ;
			//TODO
			break;
		case CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ: ;
			receive_successor_request(sd, message);
			break;
		case CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST: ;
			//TODO
			break;
		case CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST: ;
			//TODO
			break;
		case CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST: ;
			//TODO
			break;
		
		case CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP: ;
			receive_successor_response(sd, message);
			break;
		case CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_RESPONSE: ;
			//TODO
			break;
		case CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_RESPONSE: ;
			//TODO
			break;
		case CHORD_MESSAGE__MSG_NOTIFY_RESPONSE: ;
			//TODO
			break;
		default:
			exit_error("The given message didn't have a valid request set\n");
	}

	chord_message__free_unpacked(message,NULL);
	return return_value;
}

/** 
 * Does a request for the successor which should store the given node.
 * (The result will be gotten back at some point and processed with receive_successor_response.)
 * @author Adam
 * @param sd the socket for the node which requested successor; -1 if initiated by user
 * @param message Message received
 * @return return its own node if no requests are necessary (we requested from this node);
 * otherwise, return NULL (and do a request/response)
 */
Node *receive_successor_request(int sd, ChordMessage *message) {
	uint64_t id = message->r_find_succ_req->key;
	assert(message->has_query_id);
	uint32_t query_id = message->query_id;
	Node *original_node = message->r_find_succ_req->requester;

	if(n.key < id && id <= successors[0]->key) {
		// if sd == -1, then we don't need to send anything
		// because we're already at the endpoint
		if(sd == -1) {
			return &n;
		} else {	
			// Construct and send FindSuccessorResponse
			connect_send_find_successor_response(original_node, &n, query_id);
			return NULL;
		}
	} else {
		Node *nprime = closest_preceding_node(id);	
		// Get nprime's socket
		int nprime_sd = get_socket(nprime); 
		// Construct and send FindSuccessorRequest
		send_find_successor_request(nprime_sd, id, query_id);
		return NULL;
	} 
}

/**
 * After receiving, do a callback
 * @author Adam
 * @return NULL if forwarded, otherwise, the successor
 */
Node *receive_successor_response(int sd, ChordMessage *message) {
	// We received this directly from the desired node
	assert(message->msg_case == CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_RESPONSE);
	assert(message->has_query_id);

	// The callback table tells us what function to use
	// TODO
	do_callback(message);
	return NULL;
}

/**
 * Find the closest preceding node.
 * @author Adam
 * @author Gary 
 */
Node *closest_preceding_node(uint64_t id) {
	for(int i = NUM_BYTES_IDENTIFIER-1; i >= 0; i--) {
		if(n.key < finger[i]->key && finger[i]->key < id) {
			return finger[i];
		}
	}
	return &n;
}

Node **get_successor_list() {
	//TODO
	return NULL;
}

///////////////
// Auxiliary //
///////////////

/**
 * Get the socket from address_table.
 * @author Adam
 * @param nprime Node (pointer) for which we're looking to get associated socket
 * @return -1 if not found in address_table, else the socket from table.
 */
int get_socket(Node *nprime) {
	// Set up key (following the uthash guide)
	AddressTable entry;
	memset(&entry, 0, sizeof(entry));
	entry.address.sin_family = AF_INET;
	entry.address.sin_addr.s_addr = htonl(nprime->address);
	entry.address.sin_port = (u_short) htonl(nprime->port); // NOTE: copying 32 bit into 16 bit

	// Find in global variable `address_table`
	AddressTable *result;
	HASH_FIND(hh, address_table, &entry.address, sizeof(struct sockaddr_in), result);
	return ((result == NULL) ? -1 : result->sd);
}

/**
 * Given a ChordMessage, it is packed and transmitted
 * over TCP to given socket descriptor (prefixed by
 * the length of the packed ChordMessage).
 * @author Adam
 * @param sd socket to which message is sent
 * @param message pointer to message to send
 * @return -1 if failure, 0 if success
 */
int send_message(int sd, ChordMessage *message) {
	int amount_sent;

	// Pack and send message
	int64_t len = htobe64(chord_message__get_packed_size(message));
	void *buffer = malloc(len);
	chord_message__pack(message, buffer);

	// First send length, then send message
	amount_sent = send(sd, &len, sizeof(len), 0);
	assert(amount_sent == sizeof(len));

	amount_sent = send(sd, buffer, len, 0);
	assert(amount_sent == len);

	free(buffer);
	return 0;
}

/**
 * Connect to the address in request_node and send the response
 * to that node
 * @author Adam
 * @author Gary
 * @param original_node the node which first made the receive_successor_request request 
 */
void connect_send_find_successor_response(Node *original_node, Node *node_to_send, int query_id) {
	//TODO Make TCP connection
	// create new temp socket
	int original_sd = add_socket(original_node);
	send_find_successor_response(original_sd, node_to_send, query_id);
	close(original_sd);
}

/**
 * Given a socket descriptor, allocate a buffer of appropriate length,
 * get data, and unpack into a ChordMessage which is returned.
 * @author Adam
 * @param sd socket from which to read 
 * @return ChordMessage which was received from sd
 */
ChordMessage *receive_message(int sd) {
	int amount_read;

	// Read size of message
	uint64_t message_size;
	amount_read = read(sd, &message_size, sizeof(message_size));
	assert((unsigned long) amount_read == sizeof(message_size));
	// Fix endianness
	message_size = be64toh(message_size);
	
	// Read actual message
	void *buffer = malloc(message_size);
	amount_read = read(sd, buffer, message_size);
	assert((unsigned long) amount_read == message_size);

	// Unpack message
	ChordMessage *message = chord_message__unpack(NULL, message_size, buffer);
	if(message == NULL) { exit_error("Error unpacking ChordMessage\n"); }

	free(buffer);
	return message;
}

/**
 * Construct and send a ChordMessage FindSuccessorRequest.
 * @author Adam
 * @param sd Socket which we are sending to 
 * @param id ID which was identified with the socket
 * @param query_id Query ID
 */
void send_find_successor_request(int sd, int id, int query_id) {

	// Add a callback which will be referenced when we receive a final response
	int query_id = add_callback(CALLBACK_FIND_SUCCESSOR, 0);

	// Construct response
	ChordMessage message;
	RFindSuccReq request;
	chord_message__init(&message);
	r_find_succ_req__init(&request);
	// TODO do we need to free these? 
	

	message.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP;		
	request.key = id;
	message.find_successor_request = &request;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(sd, &message);
}

/**
 * Construct and send a ChordMessage FindSuccessorResponse
 */
void send_find_successor_response(int sd, Node *nprime, int query_id) {
	ChordMessage message;
	RFindSuccResp response; 
	// Not using the macros because they cause some warnings
	chord_message__init(&message);
	find_successor_response__init(&response);
	// TODO do we need to free these? 

	message.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP;		
	message.find_successor_response = &response;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(sd, &message);
}

/**
 * Create and assign the callback into the array.
 * @author Adam
 * @return The location of the callback in the callback_array (query id)
*/
int add_callback(CallbackFunction func, int arg) {
	Callback callback = {func, arg};
	int query_id = rand();
	callback_array[query_id] = callback;
	printf("Added %d, args %d -> query_id %d\n", func, arg, query_id);
	return query_id;
}

int do_callback(ChordMessage *message) {
	assert(message->has_query_id);
	Callback callback = callback_array[message->query_id];
	Node *node = message->find_successor_response->node;
	switch(callback.func) {
		case CALLBACK_FIND_SUCCESSOR: ;
			print_lookup_line(node);
			break;
		case CALLBACK_JOIN: ;
			//TODO Which successor?
			// Make a new value if it doesn't yet exist
			// and copy over the value
			if(successors[callback.arg] == NULL) {
				successors[callback.arg] = malloc(sizeof(Node));
			}
			memcpy(successors[callback.arg], node, sizeof(Node));
			break;
		case CALLBACK_FIX_FINGERS: ;
			if(finger[callback.arg] == NULL) {
				finger[callback.arg] = malloc(sizeof(Node));
			}
			memcpy(finger[callback.arg], node, sizeof(Node));			
			break;
		default: ;
			exit_error("Callback provided with unknown function enum");
	}
	
	// Remove from callback array
	//callback_array[message->query_id];
	return 0;
}

/**
 * Allocate a new memory copy of the given node.
 * @author Adam
 * @param nprime Node to copy over
 * @return Address of new node
 */
Node *copy_node(Node *nprime) {
	Node *new_node = malloc(sizeof(Node));
	memcpy(new_node, nprime, sizeof(Node));
	return new_node;
}

///////////////////////////
// Read/parse user input //
/////////////////////////// 

/**
 * Read value from standard in and process accordingly.
 * @author Adam 
 * @param fd file descriptor for standard in
 * @return 0 if success, -1 otherwise
 */
int read_process_input(FILE *fd) {
	int ret;
	// Take input, parse into command
	size_t size = 1;
	char *input = (char *) malloc(size), *command, *key;
	int bytes_read = getline(&input, &size, fd); // Assuming fd is stdin

	if(bytes_read < 0) { // read error
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

/**
 * Look up the given key and output the function 
 * @author Adam
 * @param key key to look up
 * @return 
 */
int lookup(char *key) {
	//printf("Lookup not implemented\n");
	// Get hash of key
	uint64_t key_id = get_hash(key); 

	// Display first line of output
	printf("< %s %lu\n",key,key_id);

	Node *result = receive_successor_request(-1, key_id);
	if(result != NULL) { // We already have the result, no need to wait for it
		print_lookup_line(result);
	}
	// Otherwise, we wait until we receive a result, at which point print_lookup_line
	// will be called to display the second line of the request.
	return 0;
}

/**
 * Print the second line of the lookup request.
 * Also frees the given result node.
 * @author Adam
 */
int print_lookup_line(Node *result) {
	uint64_t node_id = get_node_hash(result);
	
	// Print results
	struct in_addr ip_addr;
	ip_addr.s_addr = result->address;
	printf("< %lu %s %u\n", node_id, inet_ntoa(ip_addr), result->address);
	printf("> "); // waiting for next user input
	free(result);
	return 0;
}

//TODO
uint64_t get_node_hash(Node *n) {
	UNUSED(n);
	return -1;
}

//TODO
uint64_t get_hash(char *buffer) {
	UNUSED(buffer);
	return -1;
}

//TODO
int print_state() {
	return -1;
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
	UNUSED(cret);
	
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
 * @param server_port 
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

/**
 * stabilize as written in chord article
 * @author Gary
 * @return 1, could be made void
 */
int stabilize() {
	Node *immediate_successor = successors[0];
	Node *x = closest_preceding_node(immediate_successor->key);
	if(x->key > n.key && x->key < immediate_successor->key) {
		Node **successor_list = get_successor_list();
		for(int i = 1; i < num_successors - 1; i++) {
			successors[i] = successor_list[i-1];
		}
		successors[0] = x;
 	}
	notify(immediate_successor);
	return 1;
}

//TODO
int notify(Node *nprime) {
	UNUSED(nprime);
	return -1;
}

/**
 * fix fingers as written in chord article
 * @author Gary
 * @return 1, could be made void
 */
int fix_fingers() {
	for(int i = 0; i < NUM_BYTES_IDENTIFIER; i++) {
		Node* x = receive_successor_request(-1, n.key + (2 << (i-1)));
		if(get_socket(x) < 0) {
			// socket does not exist in the mappings/need to add it
			add_socket(x);
		} 
	}
	return 1;
}

int check_predecessor() {
	ChordMessage message;
	AddressTable *entry;
	struct sockaddr_in addr;
	int sd;

	// get socket for predecessor from global socket to address mappings
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = (unsigned short) htonl(predecessor->port);
	addr.sin_addr.s_addr = predecessor->address;

	HASH_FIND_PTR(address_table, &addr, entry);
	assert(entry);
	sd = entry->sd;

	// construct chord message check predecessor
	CheckPredecessorRequest request;
	chord_message__init(&message);
	check_predecessor_request__init(&request);
	message.msg_case = CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST;		
	message.check_predecessor_request = &request;

	send_message(sd, &message);
	// start timer
	clock_gettime(CLOCK_REALTIME, &wait_check_predecessor);

	return 0;
}

/**
 * Checks all periodic timeouts
 * @author Gary
 * @param cpp timeout for check predecessor
 * @param ffp timeout for fix fingers
 * @param sp timeout for stabilizes
 */
void check_periodic(int cpp, int ffp, int sp) {
	// check timeout
	if(check_time(&last_stabilize, sp)) {
		// stabilize()
		printf("Stabilize\n");
		fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &last_stabilize); // should go into function above
	}

	if(wait_check_predecessor.tv_sec == 0) {
		// we have no ongoing check predecessor
		if(check_time(&last_check_predecessor, cpp)) {
			// check_predecessor()
			printf("Check Predecessor\n");
			fflush(stdout);
			clock_gettime(CLOCK_REALTIME, &last_check_predecessor); // should go into function above
		}
	} else {
		if(check_time(&wait_check_predecessor, 3 * cpp)) {
			delete_socket(predecessor);
			predecessor = NULL;
		}
	}

	if(check_time(&last_fix_fingers, ffp)) {
		// fix_fingers()
		printf("Fix fingers\n");
		fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &last_fix_fingers); // should go into function above
	}
}

/**
 * Add to global address to socket mapping
 * @author Gary
 * @param n_prime Node whose address we want to map to a socket
 */
int add_socket(Node *n_prime) {
	struct sockaddr_in addr;
	int new_sock;
	AddressTable *a;
	// set up address
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = (unsigned short) htonl(n_prime->port);
	addr.sin_addr.s_addr = n_prime->address;
	// create a new socket
	if((new_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		exit_error("Could not make socket");
	}
	// connect new socket to peer
	if(connect(new_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		exit_error("Could not connect with peer");
	}
	// set up new AddressTable entry
	a = (AddressTable *) malloc(sizeof *a);
	a->address = addr;
	a->sd = new_sock;
	// add mapping to global hash map
	HASH_ADD(hh, address_table, address, sizeof(struct sockaddr_in), a);
	return new_sock;
}

/**
 * Add to global address to socket mapping
 * @author Gary
 * @param n_prime Node whose address we want to map to a socket
 */
int delete_socket(Node *n_prime) {
	struct sockaddr_in addr;
	AddressTable *ret;
	// set addr
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = (unsigned short) htonl(n_prime->port);
	addr.sin_addr.s_addr = n_prime->address;
	HASH_FIND_PTR(address_table, &addr, ret);
	if(ret) {
		// address was found remove it
		close(ret->sd);
		HASH_DEL(address_table, ret);
		free(ret);
		return 0;
	} else {
		// address was not found return -1
		return -1;
	}
}

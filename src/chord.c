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
#include "queue.h"

AddressTable *address_table;

int num_clients;
int clients[MAX_CLIENTS]; // keep track of fds, if fd is present, fds[i] = 1 else fds[i] = 0

// Num successors
uint8_t num_successors;

void printKey(uint64_t key) {
	printf("%" PRIu64, key);
}

int main(int argc, char *argv[]) {

	memset(clients, 0, MAX_CLIENTS*sizeof(int));
	num_clients = 0;

	int server_fd;
	/* Select variables */
	int maxfd = 0;
	fd_set readset;
	struct timeval timeout;
	// Parse 
	struct chord_arguments chord_args = chord_parseopt(argc, argv);
	// uint8_t num_successors = chord_args.num_successors;
	struct sockaddr_in my_address = chord_args.my_address;
	struct sockaddr_in join_address = chord_args.join_address;
	UNUSED(join_address);
	/* timeout values in seconds */
	int cpp = chord_args.check_predecessor_period;
	int ffp = chord_args.fix_fingers_period;
	int sp = chord_args.stablize_period;

	server_fd = setup_server(ntohs(my_address.sin_port));
	FD_ZERO(&readset);	// zero out readset
	FD_SET(server_fd, &readset);	// add server_fd
	FD_SET(0, &readset);	// add stdin

	init_global(chord_args);
	// node is being created
	if(chord_args.join_address.sin_port == 0) {
		// TODO: better mechanism for detecting created vs joining
		create();
	} else {
		// node is joining
		join(chord_args.join_address);
	}

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
				add_socket_to_array(client_socket);
				FD_SET(client_socket, &readset);
			}	

			if(FD_ISSET(0, &readset)) {
				// handle stdin command
				// read_process_input(server_fd);
			} 

			for(int i = 0; i < MAX_CLIENTS; i++) {
				if(clients[i] != 0 && FD_ISSET(clients[i], &readset)) {
					// process client
					// read_process_node(clients[i]);
				}
			}
			check_periodic(cpp, ffp, sp);
		} else {
			check_periodic(cpp, ffp, sp);
		}
	}

	printf("> "); // indicate we're waiting for user input
	return 0;
}

void init_global(struct chord_arguments chord_args) {
	uint8_t *hash = malloc(20);
	int cpret = clock_gettime(CLOCK_REALTIME, &last_check_predecessor);
	int ffret = clock_gettime(CLOCK_REALTIME, &last_fix_fingers);
	int spret = clock_gettime(CLOCK_REALTIME, &last_stabilize);
	UNUSED(cpret);
	UNUSED(ffret);
	UNUSED(spret);
	// set num_successors
	num_successors = chord_args.num_successors;
	// set n
	n.port = chord_args.my_address.sin_port;
	n.address = chord_args.my_address.sin_addr.s_addr;
	ctx = sha1sum_create(NULL, 0);
	sha1sum_update(ctx, (u_int8_t*)&n.address, sizeof(uint32_t));
	sha1sum_finish(ctx, (u_int8_t*)&n.port, sizeof(uint32_t), hash);
	n.key = sha1sum_truncated_head(hash);
	// initialize callback
	InitDQ(callback_list, Callback);
	assert(callback_list);
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
 */
void receive_successor_request(int sd, ChordMessage *message) {
	uint64_t id = message->r_find_succ_req->key;
	assert(message->has_query_id);
	uint32_t query_id = message->query_id;
	Node *original_node = message->r_find_succ_req->requester;

	if(n.key < id && id <= successors[0]->key) {
		// if sd == -1, then we don't need to send anything
		// because we're already at the endpoint
		if(sd == -1) {
			callback_print_lookup(&n);
		} else {	
			// Construct and send FindSuccessorResponse
			connect_send_find_successor_response(original_node, query_id);
		}
	} else {
		Node *nprime = closest_preceding_node(id);	
		// Get nprime's socket
		int nprime_sd = get_socket(nprime); 
		// Pass along the message to the next one in line
		send_message(nprime_sd, message);
	}
}

/**
 * After receiving, do a callback
 * @author Adam
 * @return NULL if forwarded, otherwise, the successor
 */
void receive_successor_response(int sd, ChordMessage *message) {
	// We received this directly from the desired node
	UNUSED(sd);
	assert(message->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP);
	assert(message->has_query_id);

	// The callback table tells us what function to use
	// TODO
	do_callback(message);
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
	//message->version = 417;

	// TODO: Check if sd is -1;
	// Pack and send message
	int64_t len = chord_message__get_packed_size(message);
	void *buffer = malloc(len);
	chord_message__pack(message, buffer);

	// First send length, then send message
	int64_t belen = htobe64(len); 
	amount_sent = send(sd, &belen, sizeof(len), 0);
	assert(amount_sent == sizeof(len));

	amount_sent = send(sd, buffer, len, 0);
	assert(amount_sent == len);

	free(buffer);
	return 0;
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
 * Start the successor 
 * @author Adam 
 * @param id ID which we are looking for
 * @param func the callback function that we're doing
 * @param arg the argument for the callback function
 */
void send_find_successor_request(uint64_t id, CallbackFunction func, int arg) {
	// TODO try other successors
	int successor_sd = get_socket(successors[0]);
	send_find_successor_request_socket(successor_sd, id, func, arg);
}

/**
 * Construct and send the *initial* ChordMessage FindSuccessorRequest.
 * The result will be caught in receive_find_successor_request.
 * @author Adam
 * @param sd Socket we send to
 * @param id ID which we are looking for
 * @param func the callback function that we're doing
 * @param arg the argument for the callback function
 */
void send_find_successor_request_socket(int sd, uint64_t id, CallbackFunction func, int arg) {

	// Add a callback which will be referenced when we receive a final response
	int query_id = add_callback(func, arg);

	// Construct response
	ChordMessage message;
	RFindSuccReq request;
	chord_message__init(&message);
	r_find_succ_req__init(&request);
	// TODO do we need to free these? 

	message.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP;		
	request.key = id;
	message.r_find_succ_req = &request;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(sd, &message);
}

/**
 * Connect to the address in request_node and send the response (i.e. the current node)
 * to request_node, with the given query_id.
 * @author Adam
 * @author Gary
 * @param original_node the node which first made the recursive FindSuccessor request 
 */
void connect_send_find_successor_response(Node *original_node, uint32_t query_id) {
	// create new temp socket
	int original_sd = add_socket(original_node);

	// send node
	ChordMessage message;
	RFindSuccResp response; 
	// Not using the macros because they cause some warnings
	chord_message__init(&message);
	r_find_succ_resp__init(&response);
	response.node = &n;
	// TODO do we need to free these? 

	message.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP;		
	message.r_find_succ_resp = &response;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(original_sd, &message);

	delete_socket(original_node);
}

/**
 * Create and assign the callback into the array.
 * @author Adam
 * @author Gary
 * @return The location of the callback in the callback_array (query id)
 */
int add_callback(CallbackFunction func, int arg) {
	int query_id = rand();
	struct Callback callback = {NULL, NULL, func, query_id, arg};
	struct Callback *cb_ptr = &callback;
	InsertDQ(callback_list, cb_ptr);
	printf("Added %d, args %d -> query_id %d\n", func, arg, query_id);
	return query_id;
}

/**
 * @author Adam
 * @author Gary
 */
int do_callback(ChordMessage *message) {
	assert(message->has_query_id);
	struct Callback *curr;
	// find callback
	for(curr = callback_list->next; curr != callback_list; curr = curr->next) {
		if(curr->query_id == message->query_id) {
			break;
		}
	}
	Node *node = message->find_successor_response->node;
	switch(curr->func) {
		case CALLBACK_PRINT_LOOKUP: ;
			callback_print_lookup(node);
			break;
		case CALLBACK_JOIN: ;
			// Set successors[callback.arg] to the given node
			callback_join(node, curr->arg);	
			break;
		case CALLBACK_FIX_FINGERS: ;
			callback_fix_fingers(node, curr->arg);
			break;
		default: ;
			exit_error("Callback provided with unknown function enum");
	}
	
	// TODO: Remove from callback array
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

	// Send a request for the given key
	send_find_successor_request(key_id, CALLBACK_PRINT_LOOKUP, 0);
	return 0;
}

/**
 * Print the second line of the lookup request.
 * Also frees the given result node.
 * @author Adam
 */
int callback_print_lookup(Node *result) {
	uint64_t node_id = get_node_hash(result);
	
	// Print results
	struct in_addr ip_addr;
	ip_addr.s_addr = result->address;
	printf("< %lu %s %u\n", node_id, inet_ntoa(ip_addr), result->address);
	printf("> "); // waiting for next user input
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
 * @param clients array of client fds already connected
 * @return new client socket
 */
int handle_connection(int sd) {
	struct sockaddr_in client_address;
	socklen_t len = sizeof(client_address);
	int client_fd = accept(sd, (struct sockaddr *)&client_address, &len);
	return client_fd;
}

////////////////////////
// Add/removing nodes //
////////////////////////

/* The structure here is that the main functions (e.g. join()) are called,
 * which call some function which will eventually add to the callback array;
 * when we get a response, we get a response which 
 */

//TODO
int join_node(Node *nprime) {
	// Assumes the key is already set in the node. 
	int nprime_sd = get_socket(nprime);
	send_find_successor_request_socket(nprime_sd, n.key, CALLBACK_JOIN, 0);
	return -1;
}

int create() {
	predecessor = NULL;
	successors[0] = &n;
	return 0;
}

//TODO
int join(struct sockaddr_in join_addr) {
	predecessor = NULL;
	Node temp_succ;
	temp_succ.address = join_addr.sin_addr.s_addr;
	temp_succ.port = join_addr.sin_port;
	successors[0] = &temp_succ;
	add_socket(&temp_succ);
	send_find_successor_request(n.key + 1, 2, 0);
	// TODO: modify to find successor list vs first successor
	return -1;
}


void callback_join(Node *node, int arg) {
	//TODO Which successor?
	// Make a new value if it doesn't yet exist
	// and copy over the value
	if(successors[arg] == NULL) {
		successors[arg] = malloc(sizeof(Node));
	}
	memcpy(successors[arg], node, sizeof(Node));
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
	ChordMessage message;
	NotifyRequest request;
	int successor_socket = get_socket(nprime);
	chord_message__init(&message);
	notify_request__init(&request);
	message.msg_case = CHORD_MESSAGE__MSG_NOTIFY_REQUEST;		
	request.node = &n;
	message.notify_request = &request;
	send_message(successor_socket, &message);
	return 0;
}

/**
 * fix fingers as written in chord article
 * @author Gary
 * @author Adam
 * @return 1, could be made void
 */
int fix_fingers() {
	// Note: the first entry of the finger table is *the current node*
	// TODO Bobby said that we said that usually we pick only
	// one at a time (randomly) to pick
	for(int i = 0; i < NUM_BYTES_IDENTIFIER; i++) {
		send_find_successor_request(n.key + (2 << (i-1)), CALLBACK_FIX_FINGERS, i); 
	}
	return 1;
}

/**
 * @author Adam
 */
void callback_fix_fingers(Node *node, int arg) {
	// Set finger[arg] to the given node
	if(finger[arg] == NULL) {
		finger[arg] = malloc(sizeof(Node));
	}
	memcpy(finger[arg], node, sizeof(Node));	

	if(get_socket(node) < 0) {
		// socket does not exist in the mappings/need to add it
		add_socket(node);
	}		
}

int check_predecessor() {
	ChordMessage message;
	
	int sd = get_socket(predecessor);
	assert(sd != -1);

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
		// printf("Stabilize\n");
		// fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &last_stabilize); // should go into function above
	}

	if(wait_check_predecessor.tv_sec == 0) {
		// we have no ongoing check predecessor
		if(check_time(&last_check_predecessor, cpp)) {
			// check_predecessor()
			// printf("Check Predecessor\n");
			// fflush(stdout);
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
		// printf("Fix fingers\n");
		// fflush(stdout);
		clock_gettime(CLOCK_REALTIME, &last_fix_fingers); // should go into function above
	}
}

/**
 * Add to global address to socket mapping
 * @author Gary
 * @param n_prime Node whose address we want to map to a socket
 * @return new socket or existing socket
 */
int add_socket(Node *n_prime) {
	struct sockaddr_in addr;
	int new_sock;
	int sd = get_socket(n_prime);
	if(sd != -1) {
		// socket already exists return it
		return sd;
	} else {
		// create a new socket
		if((new_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			exit_error("Could not make socket");
		}
		// connect new socket to peer
		if(connect(new_sock, (struct sockaddr *)&addr, sizeof(struct sockaddr *)) != 0) {
			exit_error("Could not connect with peer");
		}	
		add_socket_to_array(new_sock);
	}
	return new_sock;
}


/**
 * Add to global address to socket mapping
 * @author Gary
 * @author Adam
 * @param n_prime Node whose address we want to map to a socket
 */
int delete_socket(Node *n_prime) {
	int sd = get_socket(n_prime);
	if(sd != -1) {
		// address was found remove it
		close(sd);
		// remove from array
		delete_socket_from_array(sd);
		return 0;
	} else {
		// address was not found return -1
		return -1;
	}
}

// TODO the below function won't work because add_socket and remove_socket don't interact with this table
/**
 * Given the node (containing an address), iterate through clients
 * and look for a socket connected to that address. 
 * @author Adam
 * @return -1 if there isn't an associated socket for the given node's
 * address, else return the socket descriptor.
 */
int get_socket(Node *node) {
	//Extract address from node
	struct sockaddr_in node_address;
	memset(&node_address, 0, sizeof(node_address));

	node_address.sin_family = AF_INET;
	node_address.sin_addr.s_addr = node->address;
	node_address.sin_port = node->port;
	
	// Set up structures for iteration below
	struct sockaddr_in sd_address;
	memset(&sd_address, 0, sizeof(sd_address));
	socklen_t len;

	// Iterate over all sockets (I hope clients is set up correctly)
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i] != 0) {
			len = sizeof(sd_address);
			// Use getsockname to find the address, compare to node_address
			getsockname(clients[i], (struct sockaddr *) &sd_address, &len);
			if((sd_address.sin_addr.s_addr == node_address.sin_addr.s_addr) &&
			(sd_address.sin_port == node_address.sin_port)) {
				return clients[i];
			}
		}
	}
	// No matching socket found
	return -1;
}

/**
 * Add socket to clients 
 * @author Adam
 * References `clients`
 * @return -1 if not inserted, else the inserted socket
 */
int add_socket_to_array(int sd) {
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i] == 0) {
			clients[i] = sd;
			return clients[i];
		}
	}
	return -1;
}

/**
 * Remove socket from clients
 * @author Adam
 * @return -1 if not deleted, else the deleted socket
 */
int delete_socket_from_array(int sd) {
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i] == sd) {
			clients[i] = 0;
			return sd;
		}
	}
	return -1;
}


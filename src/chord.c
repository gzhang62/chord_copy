#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <netinet/tcp.h>
#include <netdb.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"
#include "queue.h"

#define VERBOSE false

void LOG(const char *template, ...) {
  if (VERBOSE) { 
	va_list ap;
	va_start(ap, template);
	vfprintf(stderr, template, ap);
	va_end(ap);
  }
}

int next = 0;
int num_clients;
int clients[MAX_CLIENTS]; // keep track of fds, if fd is present in clients, fds[i] = 1 else fds[i] = 0

char address_string_buffer[40]; 		// for displaying addresses
char node_string_buffer[80]; 			// for displaying nodes
char callback_string_buffer[80];		// for displaying callbacks

static char *callback_name[] = {"NONE", "PRINT_LOOKUP", "JOIN", "FIX_FINGERS", "STABILIZE_GET_PREDECESSOR", "STABILIZE_GET_SUCCESSOR_LIST"};

// TODO constant saying how long TCP connection waits before giving up
const int user_timeout = 5000;

// Num successors
uint8_t num_successors;

/**
 * Format the given address in the form "<address> <port>", store in the 
 * variable `address_string_buffer`, and return its address.
 */
char *display_address(struct sockaddr_in address) {
	memset(address_string_buffer,0,sizeof(address_string_buffer));
	sprintf(address_string_buffer, "%s %d", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
	return address_string_buffer;
}

/* Min function */
int min(int a, int b) {
	return (a > b) ? b : a;
}

/**
 * Store peer address associated with node in address_string_buffer and return its memory address.
 */
char *display_peer_address(int sd) {
	struct sockaddr_in sd_address;
	socklen_t len = sizeof(sd_address);
	getpeername(sd, (struct sockaddr *) &sd_address, &len);
	return display_address(sd_address);

}

/**
 * Store socket address associated with node in address_string_buffer and return its memory address.
 */
char *display_socket_address(int sd) {
	struct sockaddr_in sd_address;
	socklen_t len = sizeof(sd_address);
	getsockname(sd, (struct sockaddr *) &sd_address, &len);
	return display_address(sd_address);
}


/**
 * Copy the address/port from the given node into the memory address
 * at the given sockaddr_in pointer.
 */
void node_to_address(Node *node, struct sockaddr_in *out_sockaddr) {
	memset(out_sockaddr, 0, sizeof(struct sockaddr_in));
	out_sockaddr->sin_family = AF_INET;
	out_sockaddr->sin_addr.s_addr = node->address;
	out_sockaddr->sin_port = node->port;
}

/**
 * Store socket address associated with node in address_string_buffer and return its memory address.
 */
char *display_callback(CallbackFunction func, int arg) {
	char *callback_func_name = (func < sizeof(callback_name) ? callback_name[func] : "<error>");
	sprintf(callback_string_buffer, "%s(%d)", callback_func_name, arg);
	return callback_string_buffer;
}

/**
 * Format the given node in the form "<node hash> <address> <port>", store in the 
 * variable `node_string_buffer`, and return its address.
 */
char *display_node(Node *node) {
	memset(node_string_buffer,0,sizeof(node_string_buffer));
	if(node == NULL) {
		sprintf(node_string_buffer, "NULL");
	} else {
		struct sockaddr_in addr;
		node_to_address(node, &addr);
		sprintf(node_string_buffer, "%" PRIu64 " %s", node->key, display_address(addr));
	}
	return node_string_buffer;
}

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
	/* timeout values in seconds */
	int cpp = chord_args.check_predecessor_period;
	int ffp = chord_args.fix_fingers_period;
	int sp = chord_args.stablize_period;

	server_fd = setup_server(my_address.sin_port);

	init_global(chord_args);
	//printf("%d:%d\n",chord_args.join_address.sin_addr.s_addr,chord_args.join_address.sin_port);
	// node is being created
	if(join_address.sin_port == 0) {
		// TODO: better mechanism for detecting created vs joining
		create();
	} else {
		// node is joining
		join(join_address);
	}
	
	printf("> "); // indicate we're waiting for user input
	fflush(stdout); 

	for(;;) {
		FD_ZERO(&readset);				// zero out readset
		FD_SET(server_fd, &readset);	// add server_fd
		FD_SET(0, &readset);			// add stdin
		maxfd = server_fd;

		// Add all clients to readset, find maximum # clients
		for(int i = 0; i < MAX_CLIENTS; i++) {
			if(clients[i] != 0) {
				FD_SET(clients[i], &readset); 
			}
			if(clients[i] > maxfd) {
				maxfd = clients[i];
			}
		}
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int ret = select(maxfd + 1, &readset, NULL, NULL, &timeout);
		//printf("select...\n");

		if(ret == -1) {
			// error
		} else if(ret) {
			//LOG("selected\n");
			if(FD_ISSET(server_fd, &readset)) {
				// handle a new connection
				int client_socket = handle_connection(server_fd);
				add_socket_to_array(client_socket);
				FD_SET(client_socket, &readset);
			}	

			if(FD_ISSET(0, &readset)) {
				// handle input command
				read_process_input(stdin);
			} 

			for(int i = 0; i < MAX_CLIENTS; i++) {
				if(clients[i] != 0 && FD_ISSET(clients[i], &readset)) {
					// process client
					LOG("process node %d\n",clients[i]);
					read_process_node(clients[i]);
				}
			}
			check_periodic(cpp, ffp, sp);
		} else {
			check_periodic(cpp, ffp, sp);
		}
	}

	return 0;
}

void init_global(struct chord_arguments chord_args) {
	int cpret = clock_gettime(CLOCK_REALTIME, &last_check_predecessor);
	int ffret = clock_gettime(CLOCK_REALTIME, &last_fix_fingers);
	int spret = clock_gettime(CLOCK_REALTIME, &last_stabilize);
	UNUSED(cpret);
	UNUSED(ffret);
	UNUSED(spret);
	// set num_successors
	num_successors = chord_args.num_successors;


	// set n
	node__init(&n);
	n.port = chord_args.my_address.sin_port;
	// get host address
	// always uses the first entry in host_entries, I hope that's okay
	//n.address = chord_args.my_address.sin_addr.s_addr;
	char hostbuffer[256];
    if(gethostname(hostbuffer, sizeof(hostbuffer)) < 0) {
		exit_error("gethostname() failed");
	}
    struct hostent *host_entry = gethostbyname(hostbuffer);
	assert(host_entry->h_length > 0);
	struct in_addr* host_inaddr = ((struct in_addr*) host_entry->h_addr_list[0]); 
	n.address = host_inaddr->s_addr;

	ctx = sha1sum_create(NULL, 0);
	n.key = get_node_hash(&n);

	LOG("This node: {%s}\n",display_node(&n));

	// Initialize all the successors as nodes?
	/*
	for(int i = 0; i < num_successors; i++) {
		
		successors[i] = malloc(sizeof(Node));
		node__init(successors[i]);
		LOG("%p ",successors[i]);
	}
	LOG("\n");
	*/

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

	// LOG("Receive [socket %d]: ",sd);
	// LOG("%s -> ", display_peer_address(sd));
	// LOG("%s\n", display_socket_address(sd));

	ChordMessage *message = receive_message(sd);
	return_value = handle_message(sd, message);
	chord_message__free_unpacked(message,NULL);
	return return_value;
}

void receive_notify_response(int sd, ChordMessage *message) {
	UNUSED(sd);
	UNUSED(message);
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
	UNUSED(original_node);
	UNUSED(query_id);

	Node *non_failed_successor = get_non_failed_successor();
	if(non_failed_successor == NULL) {
		return;
	}
	if(in_mod_range(id, n.key+1, non_failed_successor->key)) {
		// if sd == -1, then we don't need to send anything
		// because we're already at the endpoint
		if(sd == -1) {
			LOG("callback_print_lookup\n");
			// TODO get first non-failed successor
			Node *succ = get_non_failed_successor();
			callback_print_lookup(succ);
		} else {	
			// Construct and send FindSuccessorResponse
			//LOG("connect_send_find_successor_response(%d,%" PRIu32 ")\n",original_node->key,query_id);
			connect_send_find_successor_response(message);
			// It doesn't really matter if the node fails here
		}
	} else {
		// Pass along the message to the node closest to the destination
		// Keep on trying to send until we find a node to which we can send 
		LOG("send_to_closest_preceding_node\n");
		send_to_closest_preceding_node(message);
	}
}

/** Check whether the value is between a and b mod 2^64.
 * TODO doesn't work with different sized keys.
 */
bool in_mod_range(uint64_t key, uint64_t a, uint64_t b) {
    uint64_t max_id = (uint64_t) -1;
	if(a < b) {
		return (a <= key && key <= b);
	} else { // b < a
		//return (a <= key && key <= max_id) || (0 <= key && key <= b);
		return (a <= key && key <= max_id) || (key <= b);
	}
}

/**
 * Return the first entry in successors which is not NULL.
 * Terminate if all of the successors are NULL.
*/
Node *get_non_failed_successor() {
	for(int i = 0; i < num_successors; i++) {
		if(successors[i] != NULL) {
			return successors[i];
		}
	}
	// all of the successors are null; send_successor through finger table
	LOG("All of our successors have failed");
	// for(int i = 0; i < NUM_BYTES_IDENTIFIER; i++) {
	// 	if(finger[i] != NULL) {
	// 		int sock = get_socket(finger[i]);
	// 		send_find_successor_request_socket(sock, n.key, CALLBACK_JOIN, 0);
	// 	}
	// }
	exit_error("All of our successors have failed");
	return NULL;
}

/**
 * After receiving, do a callback
 * @author Adam
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

void receive_check_predecessor_response() {
	// if we have received then predecessor is alive, zero out timestamp
	// printf("balls");
}

void receive_get_predecessor_response(int sd, ChordMessage *message) {
	UNUSED(sd);
	assert(message->msg_case == CHORD_MESSAGE__MSG_GET_PREDECESSOR_RESPONSE);
	assert(message->has_query_id);

	Node *successors_predecessor = malloc(sizeof(Node));
	memcpy(successors_predecessor, message->get_predecessor_response->node, sizeof(Node));
	// reset timestamp to indicate we have no pending get predecessor
	stabilize_get_predecessor(successors_predecessor);
	// stabilize_ongoing = 0;
}


void receive_get_successor_list_response(int sd, ChordMessage *message) {
	UNUSED(sd);
	assert(message->msg_case == CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_RESPONSE);

	int successors_list_size = message->get_successor_list_response->n_successors;
	Node **successors_list = message->get_successor_list_response->successors;

	// replace current successor list with response
	stabilize_get_successor_list(sd, successors_list, successors_list_size);
}

/**
 * Find the index into the finger table containing
 * the closest preceding node and forward the result there.
 * Loosely corresponds to closest_preceding_node from paper.
 * Return the node to which we passed along the message, or
 * NULL if something went wrong.
 * @author Adam
 * @author Gary 
 */
Node *send_to_closest_preceding_node(ChordMessage *message) {
	assert(message->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ);
	uint64_t id = message->r_find_succ_req->key;

	/* "A modified version of the closest preceding node procedure...
	   searches not only the finger table but also the successor list 
	   for the most immediate predecessor of id. In addition,
	   the pseudocode needs to be enhanced to handle node failures.
	   If a node fails during the find successor procedure, the lookup
	   proceeds, after a timeout, by trying the next best predecessor
	   among the nodes in the finger table and the successor list." "*/
	for(int i = NUM_BYTES_IDENTIFIER-1; i >= 0; i--) {
		LOG("finger[%d]->key = %" PRIu64 "\n",i,(finger[i] ? finger[i]->key : 0));
		if(finger[i] != NULL && in_mod_range(finger[i]->key, n.key+1, id-1)) {
			Node *result = send_to_entry(finger, i, message);
			if(result != NULL) {
				return result;
			}
		}
	}
	// We couldn't send it to any of the nodes in the finger table; try the successors next
	// TODO obviously redundant code
	for(int i = 0; i < num_successors; i++) {
		LOG("successors[%d]->key = %" PRIu64 "\n",i,(successors[i] ? successors[i]->key : 0));
		// Try to send the message to the node in the successor array
		Node *result = send_to_entry(successors, i, message);
		if(result != NULL) {
			return result;
		}
	}
	// At this point, we don't know any 
	return NULL;
}

/**
 * Try to pass along the message to node_array[index],
 * remove the value if it doesn't work properly.
 * Return the entry to which we passed along the message.
 * @author Adam
*/
Node *send_to_entry(Node *node_array[], int index, ChordMessage *message) {
	int nprime_sd = get_socket(node_array[index]);		
	if(nprime_sd == -2) {
		//exit_error("fingers[] or successors[] contains our own address, somehow.");
		//TODO
	} else if(nprime_sd == -1) {
		exit_error("No socket associated with entry in node_array or node_index");
	}
	int send_ret = send_message(nprime_sd, message);
	if(send_ret == -1) {
		// The node is failed, remove from the structures and move on
		free(node_array[index]);
		node_array[index] = NULL;
	}
	return node_array[index];
}

/**
 * Find the closest preceding node.
 * @author Adam
 * @author Gary 
 */
Node *closest_preceding_node(uint64_t id) {
	for(int i = NUM_BYTES_IDENTIFIER-1; i >= 0; i--) {
		if(finger[i] != NULL && (n.key < finger[i]->key && finger[i]->key < id)) {
			return finger[i];
		}
	}
	return &n;
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
	int amount_sent, ret_val = 0;
	//message->version = 417;

	if(sd == -1) {
		return -1;
	} else if(sd == -2) {
		handle_message(sd, message);
	} else {
		// Pack and send message
		int64_t len = chord_message__get_packed_size(message);
		void *buffer = malloc(len);
		chord_message__pack(message, buffer);

		// First send length...
		int64_t belen = htobe64(len); 
		amount_sent = send(sd, &belen, sizeof(len), MSG_NOSIGNAL);
		//LOG("Sent %d, tried to send %ld\n", amount_sent, sizeof(len));
		if(amount_sent != sizeof(len)) { //node failure, probably?
			LOG("socket %d failure in send_message\n",sd);
			find_delete_node_socket(sd);
			ret_val = -1;
		} else {
			// ...then send the actual message
			amount_sent = send(sd, buffer, len, MSG_NOSIGNAL);
			//LOG("Sent %d, tried to send %ld\n", amount_sent, len);
			if(amount_sent != len) {
				LOG("socket %d failure in send_message\n",sd);
				find_delete_node_socket(sd);
				ret_val = -1;				
			} else {
				// LOG("Sent [socket %d]: ",sd);
				// LOG("%s -> ", display_socket_address(sd));
				// LOG("%s\n", display_peer_address(sd));
				ret_val = 0;
			}
		}	
		free(buffer);
	}
	return ret_val;
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
	//LOG("Received %d, expected %ld\n",amount_read, sizeof(message_size));
	if(amount_read <= 0) {
		// "If no messages are available to be received and the peer has performed an orderly shutdown, 
		//recv() shall return 0"
		find_delete_node_socket(sd);
		return NULL;
	}
	// Fix endianness	
	message_size = be64toh(message_size);
	
	// Read actual message
	void *buffer = malloc(message_size);
	amount_read = read(sd, buffer, message_size);
	//LOG("Received %d, expected %ld\n",amount_read, message_size);
	if(amount_read <= 0) { 
		find_delete_node_socket(sd);
		return NULL; 
	}

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
	Node *succ = get_non_failed_successor();
	int successor_sd = get_socket(succ);
	LOG("Send Find Succ Request, id: %" PRIu64 ", callback %s, to sd %d\n", id, display_callback(func, arg));
	send_find_successor_request_socket(successor_sd, id, func, arg);
}

int send_get_predecessor_request(int sd) {
	int query_id = add_callback(CALLBACK_STABILIZE_GET_PREDECESSOR, 0);
	ChordMessage message;
	GetPredecessorRequest req;
	chord_message__init(&message);
	get_predecessor_request__init(&req);

	message.has_query_id = true;
	message.query_id = query_id;
	message.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST;
	message.get_predecessor_request = &req;

	LOG("trying to send to socket %d\n",sd);
	return send_message(sd, &message);
}

int send_get_successor_list_request(int sd) {
	//printf("send_get_successor_list_request\n");
	int query_id = add_callback(CALLBACK_STABILIZE_GET_SUCCESSOR_LIST, 0);
	ChordMessage message;
	GetSuccessorListRequest req;
	chord_message__init(&message);
	get_successor_list_request__init(&req);

	message.has_query_id = true;
	message.query_id = query_id;
	message.msg_case = CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST;
	message.get_successor_list_request = &req;

	return send_message(sd, &message);
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
	Node requester;
	chord_message__init(&message);
	r_find_succ_req__init(&request);
	node__init(&requester);
	
	// TODO do we need to free these? 
	// set node
	requester.key = n.key;
	requester.address = n.address;
	requester.port = n.port;
	request.key = id;
	request.requester = &requester;		

	message.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ;
	message.r_find_succ_req = &request;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(sd, &message);
}

/**
 * Connect to the address in request_node and send the response (i.e. the node's successor)
 * to request_node, with the given query_id.
 * @author Adam
 * @author Gary
 * @param original_node the node which first made the recursive FindSuccessor request 
 */
void connect_send_find_successor_response(ChordMessage *message_in) {
	assert(message_in->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ);
	assert(message_in->has_query_id);
	Node *original_node = message_in->r_find_succ_req->requester;
	uint64_t id = message_in->r_find_succ_req->key;
	uint32_t query_id = message_in->query_id;


	// create new temp socket, or use the previous socket if it exists
	int extant_socket = get_socket(original_node), original_sd;
	//bool socket_already_exists = (extant_socket != -1);
	if(extant_socket != -1) {
		original_sd = extant_socket;
	} else {
		original_sd = add_socket(original_node); 
		// should return existing socket if it exists, so this is a bit redundant
	}

	// send node
	ChordMessage message;
	RFindSuccResp response;
	Node node; 
	// Not using the macros because they cause some warnings
	chord_message__init(&message);
	r_find_succ_resp__init(&response);
	node__init(&node);

	// TODO use first non-failed successor  
	Node *succ = get_non_failed_successor();
	node.address = succ->address;
	node.key = succ->key;
	node.port = succ->port;

	response.node = &node;
	response.key = id;

	// TODO do we need to free these? 
	message.msg_case = CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP;		
	message.r_find_succ_resp = &response;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(original_sd, &message);

	// If we created the socket specifically for this connection, then remove it
	// if(!socket_already_exists) {
	// 	delete_socket(original_node);
	// }
}

/**
 * Send an (empty) response to the socket from which we got the
 * notify (with the given query id).
 */
int send_notify_response_socket(int sd, ChordMessage *req_mess) {
	// add requesting node to predecessor if it fits in range
	Node *node = req_mess->notify_request->node;
	
	if(predecessor == NULL || in_mod_range(node->key,predecessor->key+1,n.key-1)) {
		if(predecessor == NULL) {
			predecessor = malloc(sizeof(Node));
		}
		memcpy(predecessor,node,sizeof(Node));
	}

	ChordMessage message;
	NotifyResponse response;
	chord_message__init(&message);
	notify_response__init(&response);

	message.msg_case = CHORD_MESSAGE__MSG_NOTIFY_RESPONSE;
	message.notify_response = &response;
	message.has_query_id = true;
	message.query_id = req_mess->query_id;

	return send_message(sd, &message);
}

void send_check_predecessor_response_socket(int sd, uint32_t query_id) {
	ChordMessage message;
	CheckPredecessorResponse response;
	chord_message__init(&message);
	check_predecessor_response__init(&response);

	message.msg_case = CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_RESPONSE;
	message.check_predecessor_response = &response;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(sd, &message);
}

/**
 * Send successor list to asking socket descriptor
 * @author Gary
 * @param sd socket descriptor to send over
 */
void send_get_successor_list_response(int sd, uint32_t query_id) {
	ChordMessage message;
	GetSuccessorListResponse resp;
	chord_message__init(&message);
	get_successor_list_response__init(&resp);

	// Copy over the non-null successor entries into resp.successors
	// Determine how many entries there are we can send
	int num_non_null_successors = 0;
	for(int i = 0; i < num_successors; i++) {	
		if(successors[i] != NULL) { num_non_null_successors++; }
	}

	// Construct the successor list and copy over the non-NULL entries
	resp.n_successors = num_non_null_successors;
	resp.successors = (Node **) calloc(num_non_null_successors, sizeof(Node *));
	int j = 0;
	for(int i = 0; i < (int) resp.n_successors; i++) {
		if(successors[i] != NULL) {
			// Copy over the values into whole new memory locations and copy over values as well
			resp.successors[j] = calloc(1, sizeof(Node));
			node__init(resp.successors[j]);
			resp.successors[j]->address = successors[i]->address;
			resp.successors[j]->port = successors[i]->port;
			resp.successors[j]->key = successors[i]->key;
			j++;
		}
	}

	message.msg_case = CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_RESPONSE;
	message.get_successor_list_response = &resp;
	message.has_query_id = true;
	message.query_id = query_id;

	send_message(sd, &message);
	// Free memory
	for(int j = 0; j < (int) resp.n_successors; j++) {
		free(resp.successors[j]);
	}
	free(resp.successors);
}

/**
 * Send this node's predecessor as a message over socket sd 
 * (with the given query query_id). 
 * @author Adam
 */
int send_get_predecessor_response_socket(int sd, uint32_t query_id) {
	ChordMessage message;
	GetPredecessorResponse response;
	Node temp;
	chord_message__init(&message);
	get_predecessor_response__init(&response);
	node__init(&temp);
	
	if(predecessor == NULL) {
		temp.address = 0;
		temp.key = 0;
		temp.port = 0;
	} else {
		temp.address = predecessor->address;
		temp.port = predecessor->port;
		temp.key = predecessor->key;
	}

	response.node = &temp;

	message.msg_case = CHORD_MESSAGE__MSG_GET_PREDECESSOR_RESPONSE;
	message.get_predecessor_response = &response;
	message.has_query_id = true;
	message.query_id = query_id;

	return send_message(sd, &message);
}

/**
 * Create and assign the callback into the array.
 * @author Adam
 * @author Gary
 * @return The location of the callback in the callback_array (query id)
 */
int add_callback(CallbackFunction func, int arg) {
	int query_id = rand();
	struct Callback *cb_ptr = malloc(sizeof(struct Callback));
	cb_ptr->func = func;
	cb_ptr->arg = arg;
	cb_ptr->query_id = query_id;
	InsertDQ(callback_list, cb_ptr);
	LOG("add callback %s\n", display_callback(func, arg));
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
	Node *node = message->r_find_succ_resp->node;
	
	LOG("do callback %s\n",display_callback(curr->func,curr->arg));
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
		case CALLBACK_STABILIZE_GET_PREDECESSOR: ;
			// callback_get_predecessor();
			break;
		case CALLBACK_NONE: ;
			break;
		default: ;
			exit_error("Callback provided with unknown function enum");
	}
	
	// Remove from callback array
	DelDQ(curr);
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
	char *input = (char *) malloc(size * sizeof(char)), *command, *key;	
	int bytes_read = getline(&input, &size, fd); // Assuming fd is stdin

	if(bytes_read < 0) { // read error
		perror("Input read error encountered\n"); ret = -1;
	} else if(size <= 2) {
	    perror("No command provided\n"); ret = -1;
	} else {
    	input[size-2] = '\0'; //remove newline
    	command = strtok_r(input, " ", &key);
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
		} else if(strcmp(command, "Print") == 0){ // TODO: temp
			ret = print_predecessor();
		} else { // wrong command
			perror("Wrong command entered\n");
			ret = -1;
		}
	}
	printf(">");
	fflush(stdout);
	free(input);
	return ret;
}

/**
 * Look up the given key and output the function 
 * @author Adam
 * @param key key to look up
 * @return 0 if success, -1 if failure (theoretically)
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
	// Print results
	printf("< %s\n", display_node(result));
	return 0;
}


/**
 * Return hash of given node.
 * @author Gary
 * @author Adam
 */
uint64_t get_node_hash(Node *n) {
	uint8_t *hash = malloc(20);
	sha1sum_reset(ctx);
	sha1sum_update(ctx, (u_int8_t*)&n->address, sizeof(uint32_t));
	sha1sum_finish(ctx, (u_int8_t*)&n->port, sizeof(uint32_t), hash);
	uint64_t ret = sha1sum_truncated_head(hash);
	free(hash);
	return ret;
}

/**
 * Return hash of given data.
 * @author Adam
 */
uint64_t get_hash(char *buffer) {
	uint8_t *hash = malloc(20);
	sha1sum_reset(ctx);
	sha1sum_finish(ctx, (const uint8_t *) buffer, strlen(buffer)*sizeof(char), hash);
	uint64_t ret = sha1sum_truncated_head(hash);
	free(hash);
	return ret;
}

/**
 * Print state, according to project spec.
 */
int print_state() {
	printf("< Self %s\n", display_node(&n));
	//TODO should I start from zero or one?
	for(int i = 0; i < num_successors; i++) {
		printf("< Successor [%d] %s\n", i+1, display_node(successors[i]));
	}
	for(int i = 0; i < NUM_BYTES_IDENTIFIER; i++) {
		printf("< Finger [%d] %s\n", i+1, display_node(finger[i]));
	}
	return 0;
}

// TODO: temp
int print_predecessor() {
	printf("\nPredecessor: %s\n", display_node(predecessor));
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

	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0) {
		exit_error("setsockopt(SO_REUSEADDR) failed");
	}

	// zero out addr struct
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // TODO: change this
	server_addr.sin_port = (unsigned short) server_port;		

	// bind socket to address
	if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		exit_error("Failed to bind socket to address");
	}

	// set socket to listen
	if(listen(server_fd, MAX_CLIENTS) < 0) {
		exit_error("Failed to listen on socket");
	}
	LOG("Server setup on: {%s}\n", display_address(server_addr));
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
	LOG("taking connection from %d\n",sd);
	struct sockaddr_in client_address;
	socklen_t len = sizeof(client_address);
	int client_fd = accept(sd, (struct sockaddr *)&client_address, &len);
	// LOG("got %d/%d bytes -> socket %d, handled connection: {%s}\n", 
	// 	len, sizeof(client_address), client_fd, display_address(client_address));
	return client_fd;
}

////////////////////////
// Add/removing nodes //
////////////////////////

/* The structure here is that the main functions (e.g. join()) are called,
 * which call some function which will eventually add to the callback array;
 * when we get a response, we look into the callback array to see what we should
 * do with that data.
 */

//TODO
int join_node(Node *nprime) {
	// Assumes the key is already set in the node. 
	int nprime_sd = get_socket(nprime);
	send_find_successor_request_socket(nprime_sd, n.key, CALLBACK_JOIN, 0);
	return -1;
}

int create() {
	LOG("creating node...\n");
	predecessor = NULL;
	successors[0] = &n;
	LOG("Successor[0] @Create: %s\n", display_node(successors[0]));
	return 0;
}

//TODO
int join(struct sockaddr_in join_addr) {
	LOG("join to {%s}\n",display_address(join_addr));
	predecessor = NULL;
	Node *temp_succ = (Node *) malloc(sizeof(Node));
	temp_succ->address = join_addr.sin_addr.s_addr;
	temp_succ->port = join_addr.sin_port;
	temp_succ->key = get_node_hash(temp_succ);
	//successors[0] = temp_succ;
	LOG("temp_succ {%s}\n",display_node(temp_succ));
	int new_sd = add_socket(temp_succ);
	free(temp_succ);
	send_find_successor_request_socket(new_sd, n.key + 1, CALLBACK_JOIN, 0);
	LOG("Successor[0] @Join: %s\n", display_node(successors[0]));
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
	LOG("JOIN callback set successors[%d] to %s",arg,display_node(successors[arg]));

	// Send a request for the next node
	if(arg < num_successors - 1) {
		send_find_successor_request(node->key, CALLBACK_JOIN, arg+1);
	}
}

/**
 * stabilize after receiving a get predecessor response
 * @author Gary
 * @return 1, could be made void
 */
int stabilize_get_predecessor(Node *successor_predecessor) {
	// At this point, successors[0] should definitely exist, because we just set it in the previous function.
	Node *succ = get_non_failed_successor();
	if(successor_predecessor->port != 0	// predecessor is not null
		&& (predecessor == NULL || in_mod_range(successor_predecessor->key, n.key + 1, succ->key - 1))) {
		successors[0] = successor_predecessor;
 	} else {
		//free(successors[0]);
		successors[0] = copy_node(succ);
	}

	int sd = add_socket(successors[0]);
	send_get_successor_list_request(sd);	

	send_notify_request(successors[0]);
	return 1;
}

int stabilize_get_successor_list(int sd, Node **successors_list, uint8_t n_successors) {
	// The first entry should be the location from which this data came
	if(sd < 0) { // if the sd points to, e.g. itself (sd == -2), we don't need to do anything
		return -1;
	}

	Node *node = find_node(sd);
	Node *node_copy = copy_node(node);
	free(successors[0]);
	successors[0] = node_copy;
	// The remaining entries should be taken from the successor list.
	for(int i = 0; i < num_successors-1; i++) {
		free(successors[i+1]);
		successors[i+1] = copy_node(successors_list[i]);
	}
	// send notify
	return 0;
}

int send_notify_request(Node *nprime) {
	ChordMessage message;
	NotifyRequest request;
	Node temp_n;
	int successor_socket = get_socket(nprime);
	chord_message__init(&message);
	notify_request__init(&request);
	node__init(&temp_n);

	temp_n.address = n.address;
	temp_n.key = n.key;
	temp_n.port = n.port;

	request.node = &temp_n;
	message.msg_case = CHORD_MESSAGE__MSG_NOTIFY_REQUEST;		
	message.notify_request = &request;
	message.has_query_id = true;
	message.query_id = rand();

	// done with stabilize
	return send_message(successor_socket, &message);
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
		send_find_successor_request(n.key + (((uint64_t)1) << (i)), CALLBACK_FIX_FINGERS, i); 
	}

	// if(++next > NUM_BYTES_IDENTIFIER) {
	// 	send_find_successor_request(n.key + (((uint64_t)1) << (next)), CALLBACK_FIX_FINGERS, next); 
	// 	next = 0;
	// }
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

	if(get_socket(node) == -1) {
		// socket does not exist in the mappings/need to add it
		add_socket(node);
	}		
}

int check_predecessor() {
	ChordMessage message;
	
	if(predecessor == NULL) {
		// predecessor is already null
		return -1;
	} else {
		// construct and send a chec_predecessor message to predecessor
		int sd = get_socket(predecessor);
		if(sd == -1) { // TODO is there another case where this isn't so?
			delete_all_instances_of_node(predecessor);
			return -1;
		}	
		int query_id = rand();

		// construct chord message check predecessor
		CheckPredecessorRequest request;
		chord_message__init(&message);
		check_predecessor_request__init(&request);
		message.msg_case = CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST;		
		message.check_predecessor_request = &request;
		message.has_query_id = true;
		message.query_id = query_id;

		send_message(sd, &message);
		return 0;
	}
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
		// stabilize_ongoing = 1;
		Node *non_failed_successor = get_non_failed_successor();
		if(non_failed_successor == NULL) {
			exit_error("failed lol");
		} else {
			int sd = add_socket(non_failed_successor); // TODO: may need to wait for timeouts here
			send_get_predecessor_request(sd);  // TODO: adapt to lsit
			clock_gettime(CLOCK_REALTIME, &last_stabilize); // should go into function when stabilize completes
			
		}
		int sd = add_socket(successors[0]); // TODO: may need to wait for timeouts here
		send_get_predecessor_request(sd);  // TODO: adapt to list
		clock_gettime(CLOCK_REALTIME, &last_stabilize); // should go into function when stabilize completes
	}

	if(check_time(&last_check_predecessor, cpp)) {
		check_predecessor();
		clock_gettime(CLOCK_REALTIME, &last_check_predecessor); // should go into function above
	}

	if(check_time(&last_fix_fingers, ffp)) {
		fix_fingers();
		clock_gettime(CLOCK_REALTIME, &last_fix_fingers); // should go into function above
	}
}

// return 1 if successor has timed out, 0 otherwise
int successor_timeout(struct timespec timer, int timeout) {
	if(timer.tv_nsec != 0 && timer.tv_sec != 0 && check_time(&timer, timeout * 3)) {
		increment_failed();
		return 1;
	}
	return 0;
}

/**
 * Add to global address to socket mapping
 * @author Gary
 * @param n_prime Node whose address we want to map to a socket
 * @return new socket or existing socket; -1 if input is null
 */
int add_socket(Node *n_prime) {
	if(n_prime == NULL) {
		return -1;
	}

	// Construct address
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	//LOG("Adding socket with port %d -> %d\n",n_prime->port, (unsigned short) n_prime->port);
	addr.sin_port = (unsigned short) n_prime->port;
	addr.sin_addr.s_addr = n_prime->address;

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
		// set to be reusable 
		if(setsockopt(new_sock, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0) {
    		exit_error("setsockopt(SO_REUSEPORT) failed");
		}

		// reduce TCP timeout wait	
		// if(setsockopt(new_sock, SOL_SOCKET, TCP_USER_TIMEOUT, &user_timeout, sizeof(int)) < 0) {
    	// 	exit_error("setsockopt(TCP_USER_TIMEOUT) failed");
		// }
		// set to be nonblocking
		// https://stackoverflow.com/a/6206705/19678321
		/*
		int flags = fcntl(new_sock ,F_GETFL, 0);
		assert(flags != -1);
		if(fcntl(new_sock, F_SETFL, O_NONBLOCK) < 0) {
			exit_error("fnctl(O_NONBLOCK) failed");
		}
		*/

		LOG("socket made [socket %d]\n",new_sock);
		// connect new socket to peer
		if(connect(new_sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			exit_error("Could not connect with peer");
		}	
		LOG("connection made {%s}\n",display_address(addr));
		add_socket_to_array(new_sock);
	}
	return new_sock;
}

/**
 * Remove from global address to socket mapping
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

/**
 * Given the node (containing an address), iterate through clients
 * and look for a socket connected to that address. 
 * @author Adam
 * @return -1 if there isn't an associated socket for the given node's address, 
 * -2 if the node matches the current node, else return the socket descriptor.
 */
int get_socket(Node *node) {
	if(node == NULL) {
		return -1;
	}
	// Is this node the current node?
	// If so, return -2.
	if((node->address == n.address) && (node->port == n.port)) {
		return -2;
	}
	//Extract address from node
	struct sockaddr_in node_address;
	memset(&node_address, 0, sizeof(node_address));

	node_address.sin_family = AF_INET;
	node_address.sin_addr.s_addr = node->address;
	node_address.sin_port = node->port;

	//LOG("given: {%s}\n", display_address(node_address));

	// Set up structures for iteration below
	struct sockaddr_in sd_address;
	memset(&sd_address, 0, sizeof(sd_address));
	socklen_t len;

	// Iterate over all sockets (I hope clients is set up correctly)
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i] != 0) {
			len = sizeof(sd_address);
			// I apologize for this horrendous hack. -Adam

			// Use getsockname to find the address, compare to node_address
			getpeername(clients[i], (struct sockaddr *) &sd_address, &len);
			//LOG("clients[%d] = %d: {%s}\n", i, clients[i], display_address(sd_address));
			if((sd_address.sin_addr.s_addr == node_address.sin_addr.s_addr) &&
			(sd_address.sin_port == node_address.sin_port)) {
				//LOG("---\n");
				return clients[i];
			}

			getsockname(clients[i], (struct sockaddr *) &sd_address, &len);
			//LOG("clients[%d] = %d: {%s}\n", i, clients[i], display_address(sd_address));
			if((sd_address.sin_addr.s_addr == node_address.sin_addr.s_addr) &&
			(sd_address.sin_port == node_address.sin_port)) {
				//LOG("---\n");
				return clients[i];
			}
		}
	}
	// No matching socket found
	//LOG("---\n");
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

void increment_failed() {
	failed_successors += 1;
	if(failed_successors > num_successors) {
		exit_error("All consecutive nodes have failed");
	}
}

Node *find_node(int sd) {
	struct sockaddr_in sd_address;
	memset(&sd_address, 0, sizeof(sd_address));
	socklen_t len = sizeof(sd_address);
	getpeername(sd, (struct sockaddr *) &sd_address, &len);
	
	// try and find node in successors
	for(int i = 0; i < num_successors; i++) {
		if(	successors[i] != NULL &&
			successors[i]->address == sd_address.sin_addr.s_addr && 
			successors[i]->port == sd_address.sin_port) {
				return successors[i];
			}
	}
	// successor not found
	return NULL;
}

int handle_message(int sd, ChordMessage *message) {
if(message == NULL) { return -1; }
	// Decide what to do based on message case
	switch(message->msg_case) {
		case CHORD_MESSAGE__MSG_NOTIFY_REQUEST: ;
			send_notify_response_socket(sd, message);
			break;
		case CHORD_MESSAGE__MSG_R_FIND_SUCC_REQ: ;
			receive_successor_request(sd, message);
			break;
		case CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST: ;
			send_get_predecessor_response_socket(sd, message->query_id);
			break;
		case CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST: ;
			send_check_predecessor_response_socket(sd, message->query_id);
			break;
		case CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST: ;
			send_get_successor_list_response(sd, message->query_id);
			break;
		// Deal with responses
		case CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP: 
			receive_successor_response(sd, message);
			break;
		case CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_RESPONSE: 
			receive_check_predecessor_response();
			break;
		case CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_RESPONSE: 
			receive_get_successor_list_response(sd, message);
			break;
		case CHORD_MESSAGE__MSG_GET_PREDECESSOR_RESPONSE:
			receive_get_predecessor_response(sd, message);
			break;
		case CHORD_MESSAGE__MSG_NOTIFY_RESPONSE: ;
			//assert(message->msg_case == CHORD_MESSAGE__MSG_R_FIND_SUCC_RESP);
			//TODO we're not actually using the type of message in the responses, whoopss
			assert(message->has_query_id);	
			receive_notify_response(sd, message);
			break;
		default:
			exit_error("The given message didn't have a valid request set\n");
	}

	return 0;
}

/** Remove node from predecessor, finger, and successors. */
void delete_all_instances_of_node(Node *nprime) {
	if(nprime == NULL) {
		return;
	}

	if(predecessor != NULL) {
		if(nprime->key == predecessor->key) {
			free(predecessor);
			predecessor = NULL;
		}
	}
	// delete nprime from finger table
	for(int i = 0; i < NUM_BYTES_IDENTIFIER; i++) {
		if(finger[i] != NULL && nprime->key== finger[i]->key) {
			// key found set it to null and free it
			free(finger[i]);
			finger[i] = NULL;
		}
	}

	for(int i = 0; i < num_successors; i++) {
		if(successors[i] != NULL && nprime->key == successors[i]->key) {
			// key found set it to null and free it
			free(successors[i]);
			successors[i] = NULL;
		}
	}
}

/**
 * Remove all instances of the node associated with the socket,
 * close the socket, and remove it from the array.
 * @author Gary
 */
void find_delete_node_socket(int sd) {
	Node *nprime = find_node(sd);
	delete_all_instances_of_node(nprime);
	close(sd);
	// remove from array
	delete_socket_from_array(sd);
}

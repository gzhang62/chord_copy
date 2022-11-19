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
#include <inttypes.h>

#include "chord.pb-c.h"

// Useful Macros
#define UNUSED(x) (void)(x)

#define NUM_BYTES_IDENTIFIER 64
#define MAX_SUCCESSORS       32
#define MAX_CLIENTS          1024

typedef enum {
    CALLBACK_NONE = 0,
    CALLBACK_PRINT_LOOKUP = 1,
    CALLBACK_JOIN = 2,
    CALLBACK_FIX_FINGERS = 3,
    CALLBACK_STABILIZE_GET_PREDECESSOR = 4,
    CALLBACK_STABILIZE_GET_SUCCESSOR_LIST = 5
} CallbackFunction;

/**
 * Define what we want to do with the data after receiving a response.
 * If func equals: 
 * - CALLBACK_PRINT_LOOKUP or CALLBACK_NONE: arg is ignored
 * - CALLBACK_JOIN or CALLBACK_FIX_FINGERS: arg indicates the index 
 *   into successors[] or finger[] (respectively) we want to update
 * - [TODO]
 */
typedef struct Callback {
    struct Callback *next;
    struct Callback *prev;
    CallbackFunction func;
    int32_t query_id;
    int arg;
} Callback;

/* Timestamps for periodic update variables*/
struct timespec last_check_predecessor;
struct timespec last_fix_fingers;
struct timespec last_stabilize;
int stabilize_ongoing = 0;
int check_predecessor_ongoing = 0;
int fix_fingers_ongoing = 0;

struct sha1sum_ctx *ctx;
struct Callback *callback_list;

int failed_successors = 0; // keep track of number of failed successors

// Functions 
void parse_find_successor_request(int sd, ChordMessage *message);
void parse_find_successor_response(int sd, ChordMessage *message);
void parse_check_predecessor_response();
void parse_get_successor_list_response(int sd, ChordMessage *message);
void parse_notify_response();
void parse_get_predecessor_response(int sd, ChordMessage *message);


Node *closest_preceding_node(uint64_t id); //TODO remove
Node *send_to_closest_preceding_node(ChordMessage *message);
Node *send_to_entry(Node *node_array[], int index, ChordMessage *message);

Node **get_successor_list(); //TODO unsure if this is the best output format

// init callback linked list, node n
void init_global(struct chord_arguments chord_args);
int create();
int join(struct sockaddr_in join_addr);
void callback_join(Node *node, int arg);

int stabilize_get_predecessor();
int stabilize_get_successor_list(Node **successors_list, uint8_t n_successors);

int notify(Node *nprime);

int fix_fingers();
void callback_fix_fingers(Node *node, int arg);

int check_predecessor();

// Node interactions
int read_process_node(int sd);                      // node fds
int read_process_input(FILE *fd);                   // stdin fd



ChordMessage *smessage(int sd);

int lookup(char *key);
int print_state();
int callback_print_lookup(Node *result);

// Other
Node *copy_node(Node *nprime);

// return new server socket
int setup_server();
// add new client to known 
int handle_connection(int sd);
// check if time has elapsed, return 1 if over timeout seconds have passed since last time, 0 otherwise
int check_time(struct timespec *last_time, int timeout);
// print error message and shut down node
void exit_error(char * error_message);
// check periodic timers
void check_periodic();
// increment failed successors and check if all consecutive nodes have failed
void increment_failed();
// int successor_timeout(struct timespec timer, int timeout)

// Send/receive
int send_message(int sd, ChordMessage *message);    
ChordMessage *receive_message(int sd);

// message functions
void connect_send_find_successor_response(ChordMessage *message_in);
int send_find_successor_request_socket(int sd, uint64_t id, CallbackFunction func, int arg);
int send_get_successor_list_response(int sd, uint32_t query_id);
int send_get_successor_list_request();
int send_notify_response_socket(int sd, uint32_t query_id);
int send_get_predecessor_response_socket(int sd, uint32_t query_id);
void send_check_predecessor_response_socket(int sd, uint32_t query_id);
int send_notify_request(Node *nprime);
int send_find_successor_response(int sd, ChordMessage *message_in);

// Stateful functions
int add_callback(CallbackFunction func, int arg); 
int do_callback(ChordMessage *message);

int add_socket(Node *nprime);
int get_socket(Node *nprime);
int delete_socket(Node *nprime);

int add_socket_to_array(int sd);
int delete_socket_from_array(int sd);

// Auxiliary functions
char *display_address(struct sockaddr_in address);
void node_to_address(Node *node, struct sockaddr_in *out_sockaddr);
char *display_node(Node *node);
char *display_callback(CallbackFunction func, int arg);

uint64_t get_node_hash(Node *nprime);
uint64_t get_hash(char *buffer);

int min(int a, int b);
bool in_mod_range(uint64_t key, uint64_t a, uint64_t b);
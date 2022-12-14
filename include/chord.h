#include <inttypes.h>

#include "chord.pb-c.h"
#include "uthash.h"

// Useful Macros

#define UNUSED(x) (void)(x)

#define NUM_BYTES_IDENTIFIER 64
#define MAX_SUCCESSORS       32
#define MAX_CLIENTS          1024

// Length of a Chord Node or item key
const uint8_t KEY_LEN = 8;

/**
 * @brief Used to send messages to other Chord Nodes.
 * 
 * NOTE: Remember, you CANNOT send pointers over the network!
 */
typedef struct Message
{
    uint64_t len;
    void *ChordMessage;
} Message;

/**
 * @brief Print out the node or item key.
 * 
 * NOTE: You are not obligated to utilize this function, it is just showing
 * you how to properly print out an unsigned 64 bit integer.
 */
void printKey(uint64_t key);

uint64_t get_node_hash(Node *n);
uint64_t get_hash(char *buffer);

// Stuff we created
/* Global Variables */
Node n; // initialize on creation of node
Node *predecessor;
Node *successors[MAX_SUCCESSORS];
Node *finger[NUM_BYTES_IDENTIFIER];

/* Timestamps for periodic update variables*/
struct timespec last_check_predecessor;
struct timespec last_fix_fingers;
struct timespec last_stabilize;

struct timespec wait_check_predecessor= {0, 0}; // 0 when there is no check predecessor request going on

struct sha1sum_ctx *ctx;
struct Callback *callback_list;

// Num successors
uint8_t num_successors;

typedef enum {
    CALLBACK_NONE = 0,
    CALLBACK_PRINT_LOOKUP = 1,
    CALLBACK_JOIN = 2,
    CALLBACK_FIX_FINGERS = 3
} CallbackFunction;

/**
 * Define what we want to do with the data after receiving a response.
 * If func == CALLBACK_PRINT_LOOKUP or CALLBACK_NONE, arg is ignored
 * If == CALLBACK_JOIN or CALLBACK_FIX_FINGERS, arg indicates the index 
 * into successors[] or finger[] (respectively) we want to update
 */
typedef struct Callback {
    struct Callback *next;
    struct Callback *prev;
    CallbackFunction func;
    int32_t query_id;
    int arg;
} Callback;


// Functions 
char *display_address(struct sockaddr_in address);
void node_to_address(Node *node, struct sockaddr_in *out_sockaddr);
char *display_node(Node *node);

uint64_t get_node_hash(Node *nprime);
uint64_t get_hash(char *buffer);

void receive_successor_request(int sd, ChordMessage *message);
void receive_successor_response(int sd, ChordMessage *message);

Node *closest_preceding_node(uint64_t id);
Node **get_successor_list(); //TODO unsure if this is the best output format

// init callback linked list, node n
void init_global(struct chord_arguments chord_args);
int create();
int join(struct sockaddr_in join_addr);
void callback_join(Node *node, int arg);

int stabilize();

int notify(Node *nprime);

int fix_fingers();
void callback_fix_fingers(Node *node, int arg);

int check_predecessor();


// Node interactions
int read_process_node(int sd);                      // node fds
int read_process_input(FILE *fd);                   // stdin fd
int send_message(int sd, ChordMessage *message);    
ChordMessage *receive_message(int sd);

// message functions
void send_find_successor_request_socket(int sd, uint64_t id, CallbackFunction func, int arg);

void connect_send_find_successor_response(Node *original_node, uint32_t query_id);
void send_get_successor_list_response(int sd);
void send_successor_list_request();
void send_notify_response_socket(int sd, uint32_t query_id);
void send_get_precedessor_response_socket(int sd, uint32_t query_id);


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

// Stateful functions
int add_callback(CallbackFunction func, int arg); 
int do_callback(ChordMessage *message);

int add_socket(Node *nprime);
int get_socket(Node *nprime);
int delete_socket(Node *nprime);

int add_socket_to_array(int sd);
int delete_socket_from_array(int sd);
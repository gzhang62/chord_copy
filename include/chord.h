#include <inttypes.h>

#include "chord.pb-c.h"
#include "uthash.h"
#include "utlist.h"

// Useful Macros

#define UNUSED(x) (void)(x)

#define NUM_BYTES_IDENTIFIER 64
#define MAX_SUCCESSORS       32
#define MAX_CLIENTS          1024

// Variables

Node n; // initialize on creation of node
Node predecessor;
Node successors[MAX_SUCCESSORS];
Node finger[NUM_BYTES_IDENTIFIER];

ForwardTable *address_table;
AddressTable *forward_table;

// Provided

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

// Stuff we created

// Structures for hash map for forwarding responses to the appropriate node
// Mapping (destination node, request) -> [list_of_nodes_to_forward_to]
// TODO this will not work if the sockets change during the function
// operation; to fix this, we would need to use addresses instead of sockets
// as the keys we're pairing, then either (1) make a map from addresses to
// sockets, or (2) change to using UDP and use recv_from.

// Key in hash map
typedef struct _SocketRequest {
    int sd;
    ChordMessage__MsgCase msg_case;
} SocketRequest;

// Value in hash map (utlist)
typedef struct _ForwardSockets {
    int sd;
    struct element *prev; /* needed for a doubly-linked list only */
    struct element *next; /* needed for singly- or doubly-linked lists */
} ForwardSockets;

// Mapping struct (uthash)
typedef struct _ForwardTable {
    SocketRequest socket_request;     /* key */
    ForwardSockets *sds;              /* value */
    UT_hash_handle hh;                /* makes this structure hashable */
} ForwardTable;

// Table mapping socket address to socket
typedef struct _AddressTable {
    struct sockaddr_in address;       /* key */
    int sd;                           /* value */
    UT_hash_handle hh;                /* makes this structure hashable */
} AddressTable;

// Functions

uint64_t get_node_hash(Node *nprime);
uint64_t get_hash(char *buffer);

int add_forward(int sd_to_add);
int get_and_remove_forward(int sd_from, ChordMessage__MsgCase msg_case);

int get_socket(Node *nprime);
int add_socket(Node *nprime);

Node *find_successor(int sd, uint64_t id);
Node *closest_preceding_node(uint64_t id);
Node **get_successor_list(); //TODO unsure if this is the best output format

int create();
int join(Node *nprime);
int stabilize();
int notify(Node *nprime);
int fix_fingers();
int check_predecessor();


int read_process_node(int sd);     // node fds
int read_process_input(FILE *fd);    // stdin fd
int send_message(int sd, ChordMessage *message);
ChordMessage *receive_message(int sd);

int lookup(char *key);
int print_state();

// return new server socket
int setup_server();
// add new client to known 
int handle_connection(int sd);
// check if time has elapsed, return 1 if over timeout seconds have passed since last time, 0 otherwise
int check_time(struct timespec *last_time, int timeout);
// print error message and shut down node
void exit_error(char * error_message);



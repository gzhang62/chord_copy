#include <inttypes.h>

#include "chord.pb-c.h"

#define UNUSED(x) (void)(x)

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

typedef struct _NodeSocket {
    Node node;
    int socket;
} NodeSocket;

#define NUM_BYTES_IDENTIFIER 64
#define MAX_SUCCESSORS       32

Node n; // initialize on creation of node
Node predecessor;
Node *successors[MAX_SUCCESSORS];
Node *finger[NUM_BYTES_IDENTIFIER];

uint64_t get_node_hash(Node *n);
uint64_t get_hash(char *buffer);


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
// check periodic timers
void check_periodic();



#include <inttypes.h>

#include "chord.pb-c.h"

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


#define NUM_BYTES_IDENTIFIER 64
#define MAX_SUCCESSORS       32

Node n; // initialize on creation of node
Node predecessor;
Node successors[MAX_SUCCESSORS];
Node finger[NUM_BYTES_IDENTIFIER];

Node find_successor(uint32_t id);
Node closest_preceding_node(uint32_t id);
Node **get_successor_list(); //TODO unsure if this is the best output format

int create();
int join(Node *nprime);
int stabilize();
int notify(Node *nprime);
int fix_fingers();
int check_predecessor();


int read_process_node(int sd);     // node fds
int read_process_input(int sd);    // stdin fd

int lookup(char *key);
int print_state();

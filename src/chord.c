#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"

struct sha1sum_ctx *ctx;

void printKey(uint64_t key) {
	printf("%" PRIu64, key);
}

int main(int argc, char *argv[]) {
	ctx = sha1sum_create(NULL, 0);
	return 0;
}


/**
 * Read value from node and process accordingly.
 * @author Adam 
 * @param sd socket descriptor for node
 * @return 0 if success, -1 otherwise
 */
int read_process_node(int sd)	{
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

uint64_t get_node_hash(Node *n) {
	// Copy result IP address and port to a buffer for hashing
	uint8_t buffer[8];
	memcpy(buffer, 							(uint8_t*)&(result->address),	sizeof(result->address));
	memcpy(buffer+sizeof(result->address),	(uint8_t*)&(result->port),		sizeof(result->port));
	return get_hash(buffer);
}

uint64_t get_hash(char *buffer) {
	char checksum[20];
	int ret = sha1sum_finish(ctx, (const uint8_t*)key, strlen(key), checksum);
	uint64_t head = sha1sum_truncated_head(checksum);
	sha1sum_reset(ctx);
	return head;
}

int print_state() {
	printf("PrintState not implemented\n");
	return -1;
}

/////////////////
// Actual code //
/////////////////

Node *find_successor(uint64_t id) {
	printf("find_successor() not implemented\n");
	Node *ret = malloc(sizeof(Node));
	memset(ret,sizeof(Node),0);
	return ret;
}
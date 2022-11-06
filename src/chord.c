#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chord_arg_parser.h"
#include "chord.h"
#include "hash.h"


void printKey(uint64_t key) {
	printf("%" PRIu64, key);
}

int main(int argc, char *argv[]) {

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
	int size = 20;
	char *input = (char *) malloc(size);
	ret = getline(input, &size, fd); // Assuming fd is stdin
	char *command = strtok_r(input, " ");
	char *key = strtok_r(NULL, ""); // Read rest of line as key (should be NULL if PrintState command)
	// Determine if it's valid command / what command it is
	if(ret < 0) { // read error
		perror("Input read error encountered\n"); return -1;
	} else {
		if(strcmp(command, "Lookup") == 0) { // lookup
			if(key == NULL) {
				perror("No key passed into Lookup command"); return -1;
			} else { // do lookup
				return lookup(key);
			}
		} else if(strcmp(command, "PrintState") == 0) { // print state
			if(key != NULL) {
				perror("Extra parameter passed in to PrintState"); return -1;
			} else {
				return print_state();
			}
		} else { // wrong command
			perror("Wrong command entered\n"); return -1;
		}
	}
	
	free(input);
	return -1;
}

int lookup(char *key) {
	perror("Lookup not implemented");
	return -1;
}

int print_state() {
	perror("PrintState not implemented");
	return -1;
}
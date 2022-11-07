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
	size_t size = 1;
	char *input = (char *) malloc(size), *command, *key;
	ret = getline(&input, &size, fd); // Assuming fd is stdin
    printf("%d\n",size);
	// Determine if it's valid command / what command it is
	if(ret < 0) { // read error
		perror("Input read error encountered\n"); return -1;
	} else if(size <= 2) {
	    perror("No command provided\n"); return -1;
	} else {
    	input[size-2] = '\0'; //remove newline
    	command = strtok_r(input, " ",&key);
	    //input before space is in command, after is in key
	    //printf("COMMAND: [%s], KEY: [%s]", command, key);
	    
		if(strcmp(command, "Lookup") == 0) { // lookup
			if(strcmp(key,"") == 0) {
				perror("No key passed into Lookup command"); return -1;
			} else { // do lookup
				return lookup(key);
			}
		} else if(strcmp(command, "PrintState") == 0) { // print state
			if(strcmp(key,"") != 0) {
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
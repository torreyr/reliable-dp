#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "header.h"

/*----------------------------------------------/
 * CODE REFERENCES/HELP:
 * tcp_client.c from Lab 2:
 *		https://connex.csc.uvic.ca/access/.../tcp_client.c
 * sws.c from the first programming assignment
 *---------------------------------------------*/

// Functions
void howto();
bool checkArguments();
bool createServer();

// Global Variables
int rcv_port;

// ------- CONSOLE ------- //
void howto() {
    printf("Correct syntax: ./rdpr <receiver_ip> <receiver_port> <receiver_file_name>\n\n");
}

// ------- PARSING ------- //


// ------- SERVER ------- //
/*
 *	Checks command line arguments for correct syntax.
 */
bool checkArguments(int argc, char* argv[]) {    
	if (argc < 4) {
		printf("\nIncorrect syntax.\n");
        howto();
		return false;
	}

	// Check if valid port number.
    if (!isPort(argv[1])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else rcv_port = atoi(argv[1]);
	
	// Last argument is the file to be received.
    
	return true;
}

bool createServer() {
	struct sockaddr_in rcvaddr;
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.");
		return false;
	}
	
	rcvaddr.sin_family      = AF_INET;
    rcvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    rcvaddr.sin_port        = htons(rcv_port);
}

// MAIN
int main(int argc, char* argv[]) {
    if ( !checkArguments(argc, argv) ) return 0;
    if ( !createServer(argv) ) return 0;
}


// tc qdisc show
// tc qdisc del dev br0 root netem drop 10%
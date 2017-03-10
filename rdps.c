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
 * tcp_server.c from Lab 2:
 *		https://connex.csc.uvic.ca/access/.../tcp_server.c
 * sws.c from the first programming assignment
 *---------------------------------------------*/

// Functions
void howto();
void printStats();
bool checkArguments();
bool createServer();

// Global Variables
int sdr_port;
int rcv_port;

// ------- CONSOLE ------- //
void howto() {
    printf("Correct syntax: ./rdps <sender_ip> <sender_port> <receiver_ip> <receiver_port> <sender_file_name>\n\n");
}

void printStats() {
	
}


// ------- PARSING ------- //


// ------- SERVER ------- //
bool checkArguments(int argc, char* argv[]) {    
	if (argc < 6) {
		printf("\nIncorrect syntax.\n");
        howto();
		return false;
	}

	// Check if valid IP address.
	
	// Check if valid port number.
    if (!isPort(argv[2])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else sdr_port = atoi(argv[2]);
	
	// Check if valid IP address.
	
	// Check if valid port number.
	if (!isPort(argv[4])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else rcv_port = atoi(argv[4]);
	
	// Last argument is the file to send.
    
	return true;
}

bool createServer() {
	struct sockaddr_in sdraddr;
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.");
		return false;
	}
	
	sdraddr.sin_family      = AF_INET;
    sdraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sdraddr.sin_port        = htons(sdr_port);

}

// MAIN
int main(int argc, char* argv[]) {
    if ( !checkArguments(argc, argv) ) return 0;
    if ( !createServer(argv) ) return 0;
}

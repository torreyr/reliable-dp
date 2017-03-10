#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*----------------------------------------------/
 * CODE REFERENCES/HELP:
 * tcp_client.c from Lab 2:
 *		https://connex.csc.uvic.ca/access/.../tcp_client.c
 *---------------------------------------------*/

// Functions

// Global Variables

// ----- CONSOLE ----- //
void howto() {
    printf("Correct syntax: ./rdpr <receiver_ip> <receiver_port> <receiver_file_name>\n\n");
}

bool createServer() {
	struct sockaddr_in servaddr;
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.");
		return false;
	}
	
	sock_addr.sin_family      = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port        = htons(portnum);
}

// MAIN
int main(int argc, char* argv[]) {
    if ( !startServer(argc, argv) ) return 0;
    if ( !createServer(argv) ) return 0;
}


// tc qdisc show
// tc qdisc del dev br0 root netem drop 10%
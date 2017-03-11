/*#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

/*----------------------------------------------/
 * CODE REFERENCES/HELP:
 * tcp_client.c from Lab 2:
 *		https://connex.csc.uvic.ca/access/.../tcp_client.c
 * sws.c from the first programming assignment
 *---------------------------------------------*/

// Functions
void howto();
//void printLogMessage();
bool isPort();
char* getTime();
bool checkArguments();
bool createServer();

// Global Variables
int rcv_port;
int portnum;
char* rcv_ip;

// ------- CONSOLE ------- //
void howto() {
    printf("Correct syntax: ./rdpr <receiver_ip> <receiver_port> <receiver_file_name>\n\n");
}

/*
 *	Prints the sender's log message.
 */
 /*
void printLogMessage() {
	char* time = getTime();
    printf("%s %s:%d %s:%d\n\n", time, sdr_ip, sdr_port, rcv_ip, rcv_port);
	free(time);
}*/


// ------- PARSING ------- //
/*
 *	Validates a port number.
 */
bool isPort(char* str) {
    int portnum = atoi(str);
    if( (portnum > 0) && (portnum < 65535) ) return true;
    return false;
}


// ------- SERVER ------- //
/*
 *	Returns the formatted time of the request.
 */
char* getTime() {
    char* buffer = malloc(20);
    time_t curtime;
	struct tm* times;
	
	time(&curtime);
    times = localtime(&curtime);
    strftime(buffer, 30, "%b %d %T", times);
	return buffer;
}

/*
 *	Checks command line arguments for correct syntax.
 */
bool checkArguments(int argc, char* argv[]) {    
	if (argc < 4) {
		printf("\nIncorrect syntax.\n");
        howto();
		return false;
	}
    
	printf("\n");

	// Check if valid IP address.
	rcv_ip = argv[1];
	printf("Receiver IP:\t%s\n", rcv_ip);

	
	// Check if valid port number.
    if (!isPort(argv[2])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else rcv_port = atoi(argv[2]);
	printf("Receiver Port:\t%d\n", rcv_port);
	
	// Last argument is the file to be received.
	FILE* fp = fopen(argv[3], "r");
    
	if (fp == NULL) {
		printf("\nCould not open file. Exiting the program.\n");
        fclose(fp);
        return false;
    }
	printf("Expected file:\t%s\n", argv[3]);
    printf("\n");
	
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
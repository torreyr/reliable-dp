#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*----------------------------------------------/
 * CODE REFERENCES/HELP:
 * tcp_client.c from Lab 2:
 *		https://connex.csc.uvic.ca/access/.../tcp_client.c
 * sws.c from the first programming assignment
 * Beej's Guide to Network Programming:
 *      http://beej.us/guide/bgnet
 *---------------------------------------------*/

// Functions
void howto();
void printLogMessage();
bool isPort();
char* getTime();
bool checkArguments();
bool createServer();

// Global Variables
int sdr_port;
int rcv_port;
int portnum;
char* rcv_ip;
char* sdr_ip = NULL;
struct sockaddr_in sdraddr;

// ------- CONSOLE ------- //
void howto() {
    printf("Correct syntax: ./rdpr <receiver_ip> <receiver_port> <receiver_file_name>\n\n");
}

/*
 *	Prints the sender's log message.
 */
void printLogMessage() {
	char* time = getTime();
    printf("%s %s:%d\n\n", time, rcv_ip, rcv_port);
	free(time);
}


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
	struct timeval timenow;
	struct tm* times;
	
	time(&curtime);
    times = localtime(&curtime);
	
	gettimeofday(&timenow, NULL);
	int milli = timenow.tv_usec/1000;
	
	strftime(buffer, 30, "%T", times);
	strcat(buffer, ".");
	sprintf(buffer, "%s%d", buffer, milli);
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
	
	printf("\nSample log message:\n");
	printLogMessage();
	
	return true;
}

bool createServer() {
    fd_set fds;
	ssize_t recsize;
	struct sockaddr_in rcvaddr;
	int len  = sizeof rcvaddr;
	int slen = sizeof sdraddr;
    
	char buffer[1000];
	memset(buffer, 0, sizeof buffer );
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.");
		return false;
	}
	
	// Set socket options so that the address is reusable.
	int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(opt)) == -1) {
        printf("Problem setting socket options. Closing the socket.\n%s\n", strerror(errno));
        return false;
    }
	
    memset(&rcvaddr, 0, len);
    memset(&sdraddr, 0, slen);
	
	rcvaddr.sin_family      = AF_INET;
    rcvaddr.sin_addr.s_addr = inet_addr(rcv_ip);
    rcvaddr.sin_port        = htons(rcv_port);
	
	// Bind socket.
    if ((bind(sock, (struct sockaddr *) &rcvaddr, len)) != 0) {
        printf("Couldn't bind socket. Closing the socket.\n%s\n", strerror(errno));
        close(sock);
        return false;
    }
	
	struct timeval timeout;
	
	while (1) {
		printf("ready...\n");
		
        // Reset file descriptors and timeout.
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
		
		timeout.tv_sec = 2;
		
		if (select(sock + 1, &fds, NULL, NULL, &timeout) < 0) {   
			printf("Error with select. Closing the socket.\n");
            close(sock);
            return false;
		}
		
		if (FD_ISSET(sock, &fds)) {
			recsize = recvfrom(sock, (void*) buffer, sizeof(buffer), 0, (struct sockaddr*) &sdraddr, &slen);
		
			if (recsize <= 0) {
				printf("did not receive any data.\n");
				close(sock);
			} else {
                buffer[sizeof buffer] = '\0';
				printLogMessage();
				printf("Received: %s\n", buffer);
				
				if (sdr_ip == NULL) {
					sdr_port = ntohs(sdraddr.sin_port);
					sdr_ip   = inet_ntoa(sdraddr.sin_addr);
				}
			}
			
			memset(buffer, 0, sizeof(buffer));
		}
		
		memset(buffer, 0, sizeof(buffer));
	}
	
}

// MAIN
int main(int argc, char* argv[]) {
    if ( !checkArguments(argc, argv) ) return 0;
    if ( !createServer(argv) ) return 0;
}


// tc qdisc show
// tc qdisc del dev br0 root netem drop 10%
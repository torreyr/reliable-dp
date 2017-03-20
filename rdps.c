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
 * tcp_server.c from Lab 2:
 *		https://connex.csc.uvic.ca/access/.../tcp_server.c
 * sws.c from the first programming assignment
 * Beej's Guide to Network Programming:
 *      http://beej.us/guide/bgnet
 *---------------------------------------------*/

// Functions
void howto();
void printStats();
void printLogMessage();
bool isPort();
char* getTime();
bool checkArguments();
bool createServer();

// Global Variables
int sdr_port;
int rcv_port;
int portnum;
char* sdr_ip;
char* rcv_ip;
struct sockaddr_in rcvaddr;
struct Header {
	char magic[6];
	char type[3];
	int seq_num;
	int ack_num;
	int data_len;
	int window_size;
} header;

// ------- CONSOLE ------- //
void howto() {
    printf("Correct syntax: ./rdps <sender_ip> <sender_port> <receiver_ip> <receiver_port> <sender_file_name>\n\n");
}

void printStats() {
	
}

/*
 *	Prints the sender's log message.
 */
void printLogMessage() {
	char* time = getTime();
    printf("%s %s:%d %s:%d\n\n", time, sdr_ip, sdr_port, rcv_ip, rcv_port);
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
	struct tm* times;
	struct timeval timenow;
	
	time(&curtime);
    times = localtime(&curtime);	
	gettimeofday(&timenow, NULL);
	int micro = timenow.tv_usec;
	
	strftime(buffer, 30, "%T", times);
	strcat(buffer, ".");
	sprintf(buffer, "%s%d", buffer, micro);
	return buffer;
}

bool checkArguments(int argc, char* argv[]) {    
	printf("\n");
	
	if (argc < 6) {
		printf("Incorrect syntax.\n");
        howto();
		return false;
	}

	// Check if valid IP address.
	sdr_ip = argv[1];
	printf("Sender IP:\t%s\n", sdr_ip);
	
	// Check if valid port number.
    if (!isPort(argv[2])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else sdr_port = atoi(argv[2]);
	printf("Sender Port:\t%d\n", sdr_port);
	
	// Check if valid IP address.
	rcv_ip = argv[3];
	printf("Receiver IP:\t%s\n", rcv_ip);
	
	// Check if valid port number.
	if (!isPort(argv[4])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else rcv_port = atoi(argv[4]);
	printf("Receiver Port:\t%d\n", rcv_port);
	
	// Last argument is the file to send.
	FILE* fp = fopen(argv[5], "r");
    
	if (fp == NULL) {
		printf("\nCould not open file. Exiting the program.\n");
        fclose(fp);
        return false;
    }
	printf("Expected File:\t%s\n", argv[5]);
    
	printf("\nSample log message:\n");
	printLogMessage();
	
	return true;
}

bool createServer() {
    fd_set fds;
	struct sockaddr_in sdraddr;
    const int len = sizeof(sdraddr);
	
	printf("creating connection...\n\n");
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.");
		return false;
	}
	
	// Set socket options so that the address is reusable.
	int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(opt)) == -1) {
        printf("Problem setting socket options. Closing the socket.");
        return false;
    }
	
    memset(&sdraddr, 0, len);
	
	sdraddr.sin_family      = AF_INET;
    sdraddr.sin_addr.s_addr = inet_addr(sdr_ip);
    sdraddr.sin_port        = htons(sdr_port);
	
	// Bind socket.
    if ((bind(sock, (struct sockaddr *) &sdraddr, len)) != 0) {
        printf("Couldn't bind socket. Closing the socket.\n%s\n", strerror(errno));
        close(sock);
        return false;
    }
	
	// Reset file descriptors.
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	FD_SET(0, &fds);
	
	rcvaddr.sin_family 		= AF_INET;
	rcvaddr.sin_addr.s_addr = inet_addr(rcv_ip);
	rcvaddr.sin_port 		= htons(rcv_port);
	
	// Set the header.
	char data[1024];
	strcpy(data, "Does this work?");
	strcpy(header.magic, "CSC361");
	strcpy(header.type, "SYN");
	header.seq_num = 1;
	header.ack_num = 0;
	header.data_len = sizeof data;
	header.window_size = 10;
	
	char buffer[1024];
	strcpy(buffer, header.magic);
	strcat(buffer, header.type);
	sprintf(buffer + strlen(buffer), "%d", header.seq_num);
	sprintf(buffer + strlen(buffer), "%d", header.ack_num);
	sprintf(buffer + strlen(buffer), "%d", header.data_len);
	sprintf(buffer + strlen(buffer), "%d", header.window_size);
	strcat(buffer, data);
	
	printf("trying to send...\n");
	printf("%s\n", buffer);
	
	if ( sendto(sock, &buffer, sizeof buffer, 0, (struct sockaddr*) &rcvaddr, sizeof rcvaddr) == -1 ) {
		printf("problem sending\n");
	} else printf("successfully sent\n");
}

/*
int sendResponse(int sock, char* data) {
    int d_len = strlen(data);
	char packet[1024];

    if (d_len > 1024) {
		int offset = 0;
		
		// Create packets.
        while(offset < d_len) {
			memset(packet, 0, 1024);
			strncpy(packet, data + offset, 1024);
			if (sendto(sock, 
                       packet, 
                       1024, 
                       0, 
                       (struct sockaddr*) &client_addr, 
                       sizeof(client_addr)) == -1) return -1;
			else offset = offset + 1024;
        }
		
        return 0;
    } else return sendto(sock, data, d_len, 0, (struct sockaddr*) &client_addr, sizeof(client_addr));
}*/

// MAIN
int main(int argc, char* argv[]) {	
    if ( !checkArguments(argc, argv) ) return 0;
    if ( !createServer() ) return 0;
}

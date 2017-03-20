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
    printf("Correct syntax: ./rdpr <receiver_ip> <receiver_port> <receiver_file_name>\n\n");
}

/*
 *	Prints the sender's log message.
 */
void printLogMessage() {
	char* time = getTime();
    printf("%s %s:%d %s:%d\n", time, sdr_ip, sdr_port, rcv_ip, rcv_port);
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

/*
 *	Checks command line arguments for correct syntax.
 */
bool checkArguments(int argc, char* argv[]) {    
	printf("\n");
	
	if (argc < 4) {
		printf("Incorrect syntax.\n");
        howto();
		return false;
	}

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

/*
 *	Creates and binds the socket, and waits to receive packets.
 */
bool createServer() {
    fd_set fds;
	ssize_t recsize;
	struct sockaddr_in rcvaddr;
	int len  = sizeof rcvaddr;
	int slen = sizeof sdraddr;
    
	char buffer[1000];
	memset(buffer, 0, sizeof buffer);
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.");
		return false;
	}
	
	// Set socket options so that the address is reusable.
	int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof opt) == -1) {
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
			recsize = recvfrom(sock, (void*) buffer, sizeof buffer, 0, (struct sockaddr*) &sdraddr, &slen);
		
			if (recsize <= 0) {
				printf("did not receive any data.\n");
				close(sock);
			} else {
                buffer[sizeof buffer] = '\0';
				printf("Received: %s\n", buffer);
				
				// Set header fields.
				int i = 0;
				char tokens[6][1024];
				char* token = strtok(buffer, ",");
				while (token != NULL) {
					strcpy(tokens[i], token);
					printf("%s\n", tokens[i]);
					token = strtok(NULL, ",");
					i++;
				}
				
				printf("%s\n", tokens[1]);
				strcpy(header.type, tokens[1]);
				//header.type[3] = '\0';
				printf("%s\n", header.type);
				
				printf("%s\n", tokens[0]);
				strncpy(header.magic, tokens[0], 6);
				//header.magic[6] = '\0';
				printf("%s\n", header.magic);
				
				header.seq_num     = atoi(tokens[2]);
				header.ack_num     = atoi(tokens[3]);
				header.data_len    = atoi(tokens[4]);
				header.window_size = atoi(tokens[5]);
				strcpy(buffer, tokens[6]);
				buffer[header.data_len + 1] = '\0';
				
				printf("%s %s\n", header.magic, header.type);
				
				printf("Received stuff, split:\n%s %s %d %d %d %d %s\n", 
					header.magic,
					header.type,
					header.seq_num,
					header.ack_num,
					header.data_len,
					header.window_size,
					buffer
				);
				
				if (sdr_ip == NULL) {
					sdr_port = ntohs(sdraddr.sin_port);
					sdr_ip   = inet_ntoa(sdraddr.sin_addr);
				}
				
				printLogMessage();
			}
			
			memset(buffer, 0, sizeof buffer);
		}
		
		memset(buffer, 0, sizeof buffer);
	}
	
}

// MAIN
int main(int argc, char* argv[]) {
    if ( !checkArguments(argc, argv) ) return 0;	
    if ( !createServer(argv) ) return 0;
}


// tc qdisc show
// tc qdisc add dev br0 root netem drop 10%
// tc qdisc del dev br0 root netem drop 10%
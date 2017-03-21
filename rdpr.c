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

#define STEP(x) printf("STEP: %d\n", x); fflush(stdout);

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
void zeroHeader();
char* getTime();
void gotSyn();
bool checkArguments();
bool createServer();

// Global Constants
#define MAX_WINDOW_SIZE 1000;

// Global Variables
int sdr_port;
int rcv_port;
int portnum;
char* rcv_ip;
char* sdr_ip = NULL;
struct sockaddr_in sdraddr;
struct sockaddr_in rcvaddr;
int len  = sizeof rcvaddr;
int slen = sizeof sdraddr;

int window_size;
struct Header {
	char magic[7];
	char type[4];
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

/*
 *	Validates a port number.
 */
void zeroHeader() {
	memset(header.magic, 0, 7);
	memset(header.type, 0, 4);
	header.seq_num     = 0;
	header.ack_num     = 0;
	header.data_len    = 0;
	header.window_size = 0;
}

void setHeader(char* buffer) {
	// Tokenize received packet.
	int i = 0;
	char tokens[6][1024];
	char buf2[1000];
	strcpy(buf2, buffer);
	
	char* token = strtok(buf2, ",");
	while (token != NULL) {
		if (i == 6) {
			strncpy(tokens[i], token, atoi(tokens[4]));
			tokens[i][atoi(tokens[4])] = '\0';
		} else {
			strcpy(tokens[i], token);
		}
		token = strtok(NULL, ",");
		i++;
	}
	
	// Set header values.
	strcpy(header.magic, tokens[0]);
	strcpy(header.type, tokens[1]);		
	header.seq_num     = atoi(tokens[2]);
	header.ack_num     = atoi(tokens[3]);
	header.data_len    = atoi(tokens[4]);
	header.window_size = atoi(tokens[5]);
	
	if (i == 6) strcpy(buffer, "");
	else strcpy(buffer, tokens[6]);
}


// ------- RESPONSES ------- //
/*
 *	Called if we've received a SYN packet.
 *	Returns an ACK packet.
 */
void gotSyn(int sock, char* buffer, int buff_len) {
	printf("received a SYN packet\n");
    memset(buffer, 0, buff_len);
	sprintf(buffer, "%s,%s,%d,%d,%d,%d",
		header.magic,
		"ACK",
		header.seq_num + 1,
		header.ack_num + 1,
		0,
		window_size
	);
    
    //buffer[strlen(buffer) + 1] = '\0';
    
	printf("I have a buff len of: %d\n", buff_len);
    printf("this is what i think my buffer is: %s\n", buffer);
	if ( sendto(sock, &buffer, buff_len, 0, (struct sockaddr*) &sdraddr, slen) == -1 ) {
		printf("problem sending\n");
	} else printf("successfully sent this: %s\n", buffer);
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
    
	char buffer[1000];
	int buff_len = sizeof buffer;
	memset(buffer, 0, buff_len);
	
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
	
	window_size = MAX_WINDOW_SIZE;
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
			recsize = recvfrom(sock, (void*) buffer, buff_len, 0, (struct sockaddr*) &sdraddr, &slen);
		
			if (recsize <= 0) {
				printf("did not receive any data.\n");
				close(sock);
			} else {
                buffer[buff_len] = '\0';
				printf("Received: %s\n", buffer);
				
				zeroHeader();
				setHeader(buffer);
				
				if (sdr_ip == NULL) {
					sdr_port = ntohs(sdraddr.sin_port);
					sdr_ip   = inet_ntoa(sdraddr.sin_addr);
				}
				
				printLogMessage();
				
				// If we received a SYN, send an ACK.
				if (strcmp(header.type, "SYN") == 0) {
					gotSyn(sock, buffer, buff_len);
				}
				
			}
			
			memset(buffer, 0, buff_len);
		}
		
		memset(buffer, 0, buff_len);
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
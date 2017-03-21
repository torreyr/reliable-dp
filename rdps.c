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
int sendResponse();
bool connection();
bool createServer();

// Global Variables
FILE* fp;
int sdr_port;
int rcv_port;
int portnum;
char* sdr_ip;
char* rcv_ip;

fd_set fds;
ssize_t recsize;
struct sockaddr_in rcvaddr;
struct sockaddr_in sdraddr;
int rlen = sizeof(rcvaddr);
int len  = sizeof(sdraddr);

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
    fp = fopen(argv[5], "r");
    
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

int sendResponse(int sock) {
    // Read in the file.
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char data[4];
    
    // Walk through the file.
    do {
        printf("fp is not NULL.\n");
        fread(data, sizeof data, 1, fp);
        data[3] = '\0';
        printf("%s\n", data);
        memset(data, 0, sizeof data);
        //break;
    } while (!feof(fp));
    
}


/*
 *	Makes the initial connection between sender and receiver.
 */
bool connection(int sock) {
	
	// Set the header.	
	strcpy(header.magic, "CSC361");
	strcpy(header.type, "SYN");
	header.seq_num = rand() & 0xffff;
	header.ack_num = 0;
	header.data_len = 0;
	header.window_size = 10;
	
	// String to send.
	char buffer[1024];
	int buff_len = sizeof buffer;
	//memset(buffer, 0, buff_len);
	sprintf(buffer, "%s,%s,%d,%d,%d,%d", 
		header.magic,
		header.type,
		header.seq_num,
		header.ack_num,
		header.data_len,
		header.window_size
	);
	
	// Send packet.
	if ( sendto(sock, &buffer, buff_len, 0, (struct sockaddr*) &rcvaddr, rlen) == -1 ) {
		printf("problem sending\n");
		return false;
	} else {
		printf("successfully sent\n");
	}
	
	struct timeval timeout;
	memset(buffer, 0, buff_len);
	
	// Wait for ACK response.
	// NOTE: Can I put this while loop in its own function called waitToReceive()?
	while (1) {
		
		timeout.tv_sec = 2;
		printf("waiting for ACK...\n");
		
		if (select(sock + 1, &fds, NULL, NULL, &timeout) < 0) {   
			printf("Error with select. Closing the socket.\n");
			close(sock);
			return false;
		}
		
		if (FD_ISSET(sock, &fds)) {
			recsize = recvfrom(sock, (void*) buffer, buff_len, 0, (struct sockaddr*) &rcvaddr, &rlen);
		
			if (recsize <= 0) {
				printf("did not receive any data.\n");
				close(sock);
			} else {
				buffer[buff_len] = '\0';
				printf("Received: %s\n", buffer);
				
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
				
				if (sdr_ip == NULL) {
					sdr_port = ntohs(sdraddr.sin_port);
					sdr_ip   = inet_ntoa(sdraddr.sin_addr);
				}
				
				if (strcmp(header.type, "ACK") == 0) {
					printf("RECEIVED AN ACK!\n");
					return true;
				} else {
					printf("Received something other than an ACK.\n");
				}
				
				//printLogMessage();
			}
			
			memset(buffer, 0, buff_len);
		}
		
		memset(buffer, 0, buff_len);
	}
	
	return false;
}

bool createServer() {	
	printf("creating connection...\n\n");
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.\n");
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
	
	// Create initial connection (SYN/ACK).
	// if ( !connection(sock) ) {
		// printf("ERROR: could not make initial connection. exiting program.");
		// return false;
	// }
	
	// Set the header.
	char data[1024];
    memset(data, 0, sizeof data);
	strcpy(data, "Does this work?");
	strcpy(header.magic, "CSC361");
	strcpy(header.type, "DAT");
	header.seq_num = 1;
	header.ack_num = 0;
	header.data_len = strlen(data);
	header.window_size = 10;
	
	char buffer[1024];
	sprintf(buffer, "%s,%s,%d,%d,%d,%d,%s", 
		header.magic,
		header.type,
		header.seq_num,
		header.ack_num,
		header.data_len,
		header.window_size,
		data
	);
	
	printf("trying to send...\n");
	
	if ( sendto(sock, &buffer, sizeof buffer, 0, (struct sockaddr*) &rcvaddr, sizeof rcvaddr) == -1 ) {
		printf("problem sending\n");
	} else printf("successfully sent\n");
    
    sendResponse(sock);
}


// MAIN
int main(int argc, char* argv[]) {	
    if ( !checkArguments(argc, argv) ) return 0;
    if ( !createServer() ) return 0;
}
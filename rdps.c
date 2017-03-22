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
void zeroHeader();
void setHeader();
char* getTime();
bool checkArguments();
bool sendResponse();
bool connection();
bool createServer();

// Global Constants
#define MAX_DATA_SIZE   4
#define MAX_BUFFER_SIZE 1024
#define WINDOW_SIZE 10          // NOTE: 300 works fine. With 400, the last few packets come
                                // out of order and/or not in time. Also, doesn't like commas.

#define MAX_SYN_TIMEOUTS 150

// Global Variables
FILE* fp;
int sdr_port;
int rcv_port;
int portnum;
char* sdr_ip;
char* rcv_ip;

fd_set fds;
ssize_t recsize;
struct sockaddr_in sdraddr;
struct sockaddr_in rcvaddr;
int len  = sizeof(sdraddr);
int rlen = sizeof(rcvaddr);

int init_seq_num;
struct Header {
	char magic[7];
	char type[4];
	int seq_num;
	int ack_num;
	int data_len;
	int window_size;
} header;
bool sent_entire_file = false;


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

/*
 *	Zeros all the header values.
 */
void zeroHeader() {
	memset(header.magic, 0, 7);
	memset(header.type, 0, 4);
	header.seq_num     = 0;
	header.ack_num     = 0;
	header.data_len    = 0;
	header.window_size = 0;
}

/*
 *	Obtains the header values from a string.
 */
void setHeader(char* buffer) {
    // Tokenize received packet.
    int i = 0;
    char tokens[6][MAX_BUFFER_SIZE];
    char buf2[MAX_BUFFER_SIZE];
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
    zeroHeader();
    strcpy(header.magic, tokens[0]);
    strcpy(header.type, tokens[1]);		
    header.seq_num     = atoi(tokens[3]);
    header.ack_num     = atoi(tokens[3]);
    header.data_len    = atoi(tokens[4]);
    header.window_size = atoi(tokens[5]);
    
    if (i == 6) strcpy(buffer, "");
    else strcpy(buffer, tokens[6]);
}


// ------- RESPONSES ------- //
/*
 *  Simply sends a SYN packet.
 *  Creates a new random sequence number each time.
 */
bool sendSyn (int sock) {
    srand(time(NULL));
    init_seq_num = rand() & 0xffff;
    // Set the header.
    zeroHeader();
	header.seq_num = init_seq_num;
	
	// String to send.
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	sprintf(buffer, "%s,%s,%d,%d,%d,%d", 
		"CSC361",
		"SYN",
		header.seq_num,
		0,
		0,
		WINDOW_SIZE
	);
	
	// Send packet.    
    if ( sendto(sock, &buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &rcvaddr, rlen) == -1 ) {
		printf("Problem sending packet.\n");
        return false;
    } else {
        printf("successfully sent\n");
        return true;
    }
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

bool sendResponse(int sock, int seq) {    
    // Read in the file.
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp); 
    fseek(fp, ((seq - (init_seq_num + 1))*(MAX_DATA_SIZE - 1)), SEEK_SET);
    
    char data[MAX_DATA_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    memset(data, 0, MAX_DATA_SIZE);
    memset(buffer, 0, MAX_BUFFER_SIZE);
    
    fread(data, 1, MAX_DATA_SIZE - 1, fp);
    printf("data: %s\n", data);
    if (strcmp(data, "") == 0) {
        // reached end of file
        sent_entire_file = true;
        return false;
    }
    
    // Send the packet.
    sprintf(buffer, "%s,%s,%d,%d,%d,%d,%s", 
        header.magic,
        "DAT",
        header.seq_num,
        0,
        (int) strlen(data),
        WINDOW_SIZE,
        data
    );
    if ( sendto(sock, &buffer, sizeof buffer, 0, (struct sockaddr*) &rcvaddr, sizeof rcvaddr) == -1 ) {
        printf("Problem sending packet.\n");
        return false;
    } else printf("successfully sent\n");
    
    if (strlen(data) < (MAX_DATA_SIZE - 1)) {
        // reached end of file
        sent_entire_file = true;
        return false;
    }
    else return true;
}

bool sendData(int sock) {
    printf("trying to send...\n");
    
    int i;
    bool resp;
    for (i = 0; i < WINDOW_SIZE; i ++) {
        resp = sendResponse(sock, header.seq_num);
        if ((resp == false) && (sent_entire_file == true)) {
            return false;
        } else if ( resp == false ) {
            break;
        }
        header.seq_num += 1;
    }
    
    printf("sent_entire_file = %d\n", sent_entire_file);
            
    struct timeval timeout;
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);
    
    while (1) {
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        printf("waiting for ACK...\n");
        
        int select_return = select(sock + 1, &fds, NULL, NULL, &timeout);
        if (select_return < 0) {   
            printf("Error with select. Closing the socket.\n");
            close(sock);
            return false;
        } else if (select_return == 0) {
            printf("timeout occurred\n");
            // TODO: if max number of timeouts, give up, exit the program.
        }
        
        if (FD_ISSET(sock, &fds)) {
            recsize = recvfrom(sock, (void*) buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &sdraddr, &len);
        
            if (recsize <= 0) {
                printf("did not receive any data.\n");
                close(sock);
            } else {
                buffer[MAX_BUFFER_SIZE] = '\0';
                printf("Received: %s\n", buffer);
                
                zeroHeader();
                setHeader(buffer);
                
                if (strcmp(header.type, "ACK") == 0) {
                    printf("RECEIVED AN ACK!\n");
                    return true;
                } else {
                    printf("Received something other than an ACK.\n");
                }
                
                printLogMessage();
            }
            
            memset(buffer, 0, MAX_BUFFER_SIZE);
        }
        
        memset(buffer, 0, MAX_BUFFER_SIZE);
    }
    
    return true;
}

/*
 *	Makes the initial connection between sender and receiver.
 */
bool connection(int sock) {

    // Send initial SYN packet.
    if ( sendSyn(sock) == false ) {
        return false;
    }

    int syn_timeouts = 0;
	struct timeval timeout;
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	
	// Wait for ACK response.
	// NOTE: Can I put this while loop in its own function called waitToReceive()?
	while (1) {
		
		timeout.tv_sec = 2;
        timeout.tv_usec = 0;
		printf("waiting for ACK...\n");
		
        int select_return = select(sock + 1, &fds, NULL, NULL, &timeout);
		if (select_return < 0) {   
			printf("Error with select. Closing the socket.\n");
			close(sock);
			return false;
		} else if (select_return == 0) {
            printf("timeout occurred\n");
            syn_timeouts++;
            if (syn_timeouts == MAX_SYN_TIMEOUTS) {
                printf("ERROR: Connection request timed out too many times.\n");
                return false;
            } else if ( sendSyn(sock) == false) {
                return false;
            }
        }
		
		if (FD_ISSET(sock, &fds)) {
			recsize = recvfrom(sock, (void*) buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &sdraddr, &len);
		
			if (recsize <= 0) {
				printf("did not receive any data.\n");
				close(sock);
			} else {
				buffer[MAX_BUFFER_SIZE] = '\0';
				printf("Received: %s\n", buffer);
                
                zeroHeader();
                setHeader(buffer);
				
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
				
				printLogMessage();
			}
			
			memset(buffer, 0, MAX_BUFFER_SIZE);
		}
		
		memset(buffer, 0, MAX_BUFFER_SIZE);
	}
	
	return false;
}

bool createServer() {	
	printf("creating connection...\n\n");
	
	// Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		printf("Couldn't create socket. Exiting the program.\n");
        close(sock);
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
	if ( !connection(sock) ) {
		printf("ERROR: Could not make initial connection. Exiting program.\n");
		return false;
	}
    
    // Send the data.
    while (sent_entire_file == false) {
        if ( sendData(sock) == false ) return false;
    }
}


// MAIN
int main(int argc, char* argv[]) {	
    if ( !checkArguments(argc, argv) ) return 0;
    if ( !createServer() ) return 0;
    fclose(fp);
}
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
#define MAX_DATA_SIZE   500
#define MAX_BUFFER_SIZE 1024
#define WINDOW_SIZE     15

#define MAX_TIMEOUTS    150
#define TIMEOUT_SEC     0
#define TIMEOUT_USEC    50000

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
int expected_ack_num;
int expected_fin_ack_num;
struct Header {
	char magic[7];
	char type[4];
	int seq_num;
	int ack_num;
	int data_len;
	int window_size;
} header;
bool sent_entire_file = false;
bool done_sending_file = false;
bool problem = false;
int window_size = WINDOW_SIZE;
//int last_window_bytes;

// Global Variables for Stats
int t_bytes = 0;
int u_bytes = 0;
int t_packs = 0;
int u_packs = 0;
int uniques = 0;
int syns_sent = 0;
int fins_sent = 0;
int rsts_sent = 0;
int acks_recv = 0;
int rsts_recv = 0;
int start_time = 0;
int end_time = 0;


// ------- CONSOLE ------- //
void howto() {
    printf("Correct syntax: ./rdps <sender_ip> <sender_port> <receiver_ip> <receiver_port> <sender_file_name>\n\n");
}

void printStats() {    
	printf(
        "\ntotal data bytes sent: %d\n"
        "unique data bytes sent: %d\n"
        "total data packets sent: %d\n"
        "unique data packets sent: %d\n"
        "SYN packets sent: %d\n"
        "FIN packets sent: %d\n"
        "RST packets sent: %d\n"
        "ACK packets received: %d\n"
        "RST packets received: %d\n"
        "total time duration (seconds): %d\n",
        t_bytes, u_bytes, t_packs, u_packs,
        syns_sent, fins_sent, rsts_sent,
        acks_recv, rsts_recv, end_time - start_time
    );
}

/*
 *	Prints the sender's log message.
 */
void printLogMessage(char* event_type) {
	char* time = getTime();
    printf("%s %s %s:%d %s:%d %s %d %d\n",
        time, 
        event_type,
        rcv_ip, rcv_port, 
        sdr_ip, sdr_port, 
        header.type, 
        header.seq_num, 
        window_size
    );
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
    char tokens[7][MAX_BUFFER_SIZE];
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
	
    strcpy(header.type, "SYN");
    printLogMessage("s");
    
	// Send packet.    
    if ( sendto(sock, &buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &rcvaddr, rlen) == -1 ) {
		printf("Problem sending packet.\n");
        return false;
    } else {
        //printf("successfully sent\n");
        syns_sent++;
        return true;
    }
}

/*
 *  Simply sends a FIN packet.
 */
bool sendFin (int sock) {
    window_size = WINDOW_SIZE;

	// String to send.
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	sprintf(buffer, "%s,%s,%d,%d,%d,%d", 
		"CSC361",
		"FIN",
		expected_ack_num,
		0,
		0,
		window_size
	);
	
    strcpy(header.type, "FIN");
    printLogMessage("s");
    
	// Send packet.    
    if ( sendto(sock, &buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &rcvaddr, rlen) == -1 ) {
		printf("Problem sending packet.\n");
        return false;
    } else {
        //printf("successfully sent\n");
        fins_sent++;
        return true;
    }
}

// ------- SERVER ------- //
/*
 *	Returns the formatted time of the request.
 */
char* getTime() {
    char* timebuf = malloc(20);
    time_t curtime;
	struct tm* times;
	struct timeval timenow;
	
	time(&curtime);
    times = localtime(&curtime);	
	gettimeofday(&timenow, NULL);
	int micro = timenow.tv_usec;
	
	strftime(timebuf, 30, "%T", times);
	strcat(timebuf, ".");
	sprintf(timebuf, "%s%d", timebuf, micro);
	return timebuf;
}

bool checkArguments(int argc, char* argv[]) {    
	//printf("\n");
	
	if (argc < 6) {
		printf("Incorrect syntax.\n");
        howto();
		return false;
	}

	// Check if valid IP address.
	sdr_ip = argv[1];
	//printf("Sender IP:\t%s\n", sdr_ip);
	
	// Check if valid port number.
    if (!isPort(argv[2])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else sdr_port = atoi(argv[2]);
	//printf("Sender Port:\t%d\n", sdr_port);
	
	// Check if valid IP address.
	rcv_ip = argv[3];
	//printf("Receiver IP:\t%s\n", rcv_ip);
	
	// Check if valid port number.
	if (!isPort(argv[4])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else rcv_port = atoi(argv[4]);
	//printf("Receiver Port:\t%d\n", rcv_port);
	
	// Last argument is the file to send.
    fp = fopen(argv[5], "r");
    
	if (fp == NULL) {
		printf("\nCould not open file. Exiting the program.\n");
        fclose(fp);
        return false;
    }
	//printf("Expected File:\t%s\n", argv[5]);
    
	//printf("\nSample log message:\n");
	//printLogMessage();
	
	return true;
}

/*
 *  Sends a DAT packet.
 */
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
    //printf("data: %s\n", data);
    if (strcmp(data, "") == 0) {
        // reached end of file
        sent_entire_file = true;
        return false;
    }
    
    // keep track of bytes of last WINDOW_SIZE number of packets.
    // if (window_size > 0) {
        // last_window_bytes += strlen(data);
    // }
    // if the ack is the expected number
    // if ((window_size == 1) && (header.ack_num != expected_ack_num)) {
        // u_bytes += last_window_bytes;
        // u_packs += WINDOW_SIZE;
        
        // reset num of bytes.
        
        // printf("did not get expected ack number\n");
        // printf("WINDOW_SIZE - window_size = %d\n", WINDOW_SIZE - window_size);
        // u_packs += WINDOW_SIZE - window_size;
        // printf("u_packs = %d\n", u_packs);
    // }
    
    window_size = window_size - 1;
    
    // Send the packet.
    expected_ack_num = header.seq_num + 1;
    sprintf(buffer, "%s,%s,%d,%d,%d,%d,%s", 
        header.magic,
        "DAT",
        header.seq_num,
        0,
        (int) strlen(data),
        window_size,
        data
    );
    if ( sendto(sock, &buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &rcvaddr, sizeof rcvaddr) == -1 ) {
        printf("Problem sending packet.\n");
        problem = true;
        return false;
    }// else printf("successfully sent\n");
    
    strcpy(header.type, "DAT");
    printLogMessage("s");
    if (window_size == 0) window_size = WINDOW_SIZE;
    t_bytes += strlen(data);
    t_packs++;
    
    if (strlen(data) < (MAX_DATA_SIZE - 1)) {
        // reached end of file
        sent_entire_file = true;
        return false;
    } else {
        return true;
    }
}
/*
 *  Sends the file.
 */
bool sendData(int sock) {
    printf("sending next window sized chunk...\n");
    
    int i;
    bool resp;
    int seq_num = init_seq_num;
    for (i = 0; i < WINDOW_SIZE; i ++) {
        if (i == 0) seq_num = header.seq_num;
        resp = sendResponse(sock, header.seq_num);
        if ( resp == false ) {
            break;
        }
        header.seq_num += 1;
    }
    
    //printf("sent_entire_file = %s\n", sent_entire_file ? "true" : "false");
	
    int timeouts = 0;
	int select_return;
    struct timeval timeout;
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);
    
    while (1) {
		// Reset file descriptors.
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		FD_SET(0, &fds);
		
		timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;
        //printf("waiting for ACK...\n");
        
        select_return = select(sock + 1, &fds, NULL, NULL, &timeout);
        if (select_return < 0) {   
            printf("Error with select. Closing the socket.\n");
            close(sock);
            problem = true;
            return false;
        } else if (select_return == 0) {
            printf("timeout occurred\n");
            timeouts++;
            if (timeouts == MAX_TIMEOUTS) {
                printf("ERROR: Connection request timed out too many times.\n");
                close(sock);
                problem = true;
                return false;
            }
        }
        
        if (FD_ISSET(sock, &fds)) {
            recsize = recvfrom(sock, (void*) buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &sdraddr, &len);
        
            if (recsize <= 0) {
                printf("did not receive any data.\n");
                close(sock);
                problem = true;
                return false;
            } else {
                buffer[MAX_BUFFER_SIZE] = '\0';
                //printf("Received: %s\n", buffer);
                
                zeroHeader();
                setHeader(buffer);
                timeouts = 0;
                
                if (strcmp(header.type, "ACK") == 0) {
                    //printf("RECEIVED AN ACK!\n");                    
                    printLogMessage("r");
                    acks_recv++;


                    // if (header.ack_num != expected_ack_num) {                        
                        // printf("did not get expected ack number\n");
                        // uniques = WINDOW_SIZE - (seq_num + 10 - header.ack_num);
                        
                        // non-uniques = WINDOW_SIZE - uniques
                        // (seq_num + non-uniques - header.ack_num)
                        
                        // printf("uniques = %d\n", uniques);
                        // u_packs += uniques;
                        // printf("u_packs = %d\n", u_packs);
                    // }


                    if (sent_entire_file == false) return true;
                    else if (header.ack_num == expected_ack_num) {
                        done_sending_file = true;
                        return false;
                    } else return true;
                    
                } else {
                    printf("Received something other than an ACK.\n");
                }
                
                printLogMessage("r");
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
		// Reset file descriptors.
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		FD_SET(0, &fds);
		
		timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;
		printf("waiting for ACK...\n");
		
        int select_return = select(sock + 1, &fds, NULL, NULL, &timeout);
		if (select_return < 0) {   
			printf("Error with select. Closing the socket.\n");
			close(sock);
			return false;
		} else if (select_return == 0) {
            printf("timeout occurred\n");
            syn_timeouts++;
            if (syn_timeouts == MAX_TIMEOUTS) {
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
				//printf("Received: %s\n", buffer);
                
                zeroHeader();
                setHeader(buffer);
				
				if (sdr_ip == NULL) {
					sdr_port = ntohs(sdraddr.sin_port);
					sdr_ip   = inet_ntoa(sdraddr.sin_addr);
				}
				
				if (strcmp(header.type, "ACK") == 0) {
					//printf("RECEIVED AN ACK!\n");
                    printLogMessage("r");
                    acks_recv++;
					return true;
				} else {
					printf("Received something other than an ACK.\n");
				}
				
				printLogMessage("r");
			}
			
			memset(buffer, 0, MAX_BUFFER_SIZE);
		}
		
		memset(buffer, 0, MAX_BUFFER_SIZE);
	}
	
	return false;
}

/*
 *	Closes the connection between sender and receiver.
 */
bool closing(int sock) {
    // Send initial FIN packet.
    if ( sendFin(sock) == false ) {
        return false;
    }
    
    //printf("expected_fin_ack_num = %d\n", expected_fin_ack_num);

    int timeouts = 0;
	struct timeval timeout;
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	
	// Wait for ACK response.
	// NOTE: Can I put this while loop in its own function called waitToReceive()?
	while (1) {
		// Reset file descriptors.
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		FD_SET(0, &fds);
		
		timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;
		//printf("waiting for ACK...\n");
		
        int select_return = select(sock + 1, &fds, NULL, NULL, &timeout);
		if (select_return < 0) {   
			printf("Error with select. Closing the socket.\n");
			close(sock);
			return false;
		} else if (select_return == 0) {
            printf("timeout occurred\n");
            timeouts++;
            if (timeouts == MAX_TIMEOUTS) {
                printf("ERROR: Connection request timed out too many times.\n");
                return false;
            } else if ( sendFin(sock) == false) {
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
				//printf("Received: %s\n", buffer);
                
                zeroHeader();
                setHeader(buffer);
				
				if (strcmp(header.type, "ACK") == 0) {
					//printf("RECEIVED AN ACK!\n");
                    acks_recv++;
                    if (header.ack_num == expected_fin_ack_num) {
                        printf("Closing the connection.\n");
                        return true;
                    } else sendFin(sock);
				} else {
					printf("Received something other than an ACK.\n");
				}
				
				printLogMessage("r");
			}
			
			memset(buffer, 0, MAX_BUFFER_SIZE);
		}
		
		memset(buffer, 0, MAX_BUFFER_SIZE);
	}
	
	return false;
}

bool createServer() {	
	//printf("creating connection...\n\n");
	
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
	
    start_time = time(NULL);
    
	// Create initial connection (SYN/ACK).
	if ( !connection(sock) ) {
		printf("ERROR: Could not make initial connection. Exiting program.\n");
		return false;
	}
    
    // Send the data.
    while (done_sending_file == false) {
        if ( (sendData(sock) == false) && (problem == false) ) break;
        else if (problem == true) return false;
    }
    
    //printf("sent_entire_file = %d\n", sent_entire_file);
    
    // Close the connection (FIN/ACK).
    expected_fin_ack_num = expected_ack_num + 1;
    if ( !closing(sock) ) {
		printf("ERROR: Could not gracefully close the connection. Exiting program.\n");
		return false;
	}
    
    end_time = time(NULL);
    return true;
}


// MAIN
int main(int argc, char* argv[]) {	
    if ( !checkArguments(argc, argv) ) return 0;
    if ( !createServer() ) return 0;
    fclose(fp);
    printStats();
}
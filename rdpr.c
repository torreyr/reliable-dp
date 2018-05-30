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
 *      https://connex.csc.uvic.ca/access/.../tcp_client.c
 * sws.c from the first programming assignment
 * Beej's Guide to Network Programming:
 *      http://beej.us/guide/bgnet
 *---------------------------------------------*/

// Function Prototypes
void howto();
void printLogMessage();
bool isPort();
void zeroHeader();
char* getTime();
void gotSyn();
bool checkArguments();
bool createServer();

// Global Constants
#define MAX_BUFFER_SIZE 1024
#define MAX_WINDOW_SIZE 10

#define MAX_TIMEOUTS    100
#define TIMEOUT_SEC     0
#define TIMEOUT_USEC    5000

// Global Variables
FILE* fp;
int sdr_port;
int rcv_port;
int portnum;
char* rcv_ip;
char* sdr_ip = NULL;

struct sockaddr_in rcvaddr;
struct sockaddr_in sdraddr;
int len  = sizeof rcvaddr;
int slen = sizeof sdraddr;

int ack_num;
int expected_seq_num;
struct Header {
    char magic[7];
    char type[4];
    int seq_num;
    int ack_num;
    int data_len;
    int window_size;
} header;
int window_size = MAX_WINDOW_SIZE;
bool connected = false;
bool received_fin = false;
int num_received = 0;

// Global Variables for Stats
int t_bytes = 0;
int u_bytes = 0;
int t_packs = 0;
int u_packs = 0;
int syns_recv = 0;
int fins_recv = 0;
int rsts_recv = 0;
int acks_sent = 0;
int rsts_sent = 0;
int start_time = 0;
int end_time = 0;

// ------- CONSOLE ------- //
void howto() {
    printf("Correct syntax: ./rdpr <receiver_ip> <receiver_port> <receiver_file_name>\n\n");
}

void printStats() {
    printf(
        "\ntotal data bytes received: %d\n"
        "unique data bytes received: %d\n"
        "total data packets received: %d\n"
        "unique data packets received: %d\n"
        "SYN packets received: %d\n"
        "FIN packets received: %d\n"
        "RST packets received: %d\n"
        "ACK packets sent: %d\n"
        "RST packets sent: %d\n"
        "total time duration (seconds): %d\n",
        t_bytes, u_bytes, t_packs, u_packs,
        syns_recv, fins_recv, rsts_recv,
        acks_sent, rsts_sent, end_time - start_time
    );
}

/*
 *  Prints the sender's log message.
 */
void printLogMessage(int seq, char* event_type) {
    char* time = getTime();
    int num = seq ? header.seq_num : ack_num;
    printf("%s %s %s:%d %s:%d %s %d %d\n", 
        time,
        event_type,
        sdr_ip, sdr_port,
        rcv_ip, rcv_port,
        header.type,
        num,
        window_size
    );
    free(time);
}


// ------- PARSING ------- //
/*
 *  Validates a port number.
 */
bool isPort(char* str) {
    int portnum = atoi(str);
    if( (portnum > 0) && (portnum < 65535) ) return true;
    return false;
}

/*
 *  Zeros all the header values.
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
 *  Obtains the header values from a string.
 */
void setHeader(char buffer[]) {
    char tmp[MAX_BUFFER_SIZE];
    int i = 0;
    int j = 0;
    int token = 0;
    for(; token < 6 && i < MAX_BUFFER_SIZE; i++) {
        if(buffer[i] == ',') {
            memset(tmp, 0, MAX_BUFFER_SIZE);
            strncpy(tmp, buffer + j, i - j);
            switch(token) {
                case 0: strcpy(header.magic, tmp); break;
                case 1: strcpy(header.type, tmp); break;
                case 2: header.seq_num = atoi(tmp); break;
                case 3: header.ack_num = atoi(tmp); break;
                case 4: header.data_len = atoi(tmp); break;
                case 5: header.window_size = atoi(tmp); break;
            }
            j = i + 1;
            token++;
        }
    }
    strcpy(tmp, buffer + j);
    strcpy(buffer, tmp);
}

void printToFile(char* buffer) {
    fwrite(buffer, 1, header.data_len, fp);
}


// ------- RESPONSES ------- //
/*
 *  Simply sends an ACK packet.
 */
void sendAck(int sock, char* buffer) {
    memset(buffer, 0, MAX_BUFFER_SIZE);
    sprintf(buffer, "%s,%s,%d,%d,%d,%d",
        header.magic,
        "ACK",
        0,
        ack_num,
        0,
        window_size
    );
    
    strcpy(header.type, "ACK");
    acks_sent++;
    printLogMessage(0, "s");
    
    if ( sendto(sock, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &sdraddr, slen) == -1 ) {
        printf("problem sending\n");
    }
}


// ------- SERVER ------- //
/*
 *  Returns the formatted time of the request.
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

/*
 *  Checks command line arguments for correct syntax.
 */
bool checkArguments(int argc, char* argv[]) {    
    if (argc < 4) {
        printf("Incorrect syntax.\n");
        howto();
        return false;
    }

    // Check if valid IP address.
    rcv_ip = argv[1];

    // Check if valid port number.
    if (!isPort(argv[2])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    } else rcv_port = atoi(argv[2]);
    
    // Last argument is the file to be received.
    fp = fopen(argv[3], "w");
    
    return true;
}

/*
 *  Creates and binds the socket, and waits to receive packets.
 */
bool createServer() {
    fd_set fds;
    ssize_t recsize;
    
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);
    
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
    
    int timeouts = 0;
    struct timeval timeout;
    start_time = time(NULL);
    
    printf("ready...\n");

    while (1) {
        
        // Reset file descriptors and timeout.
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = TIMEOUT_USEC;
        
        int select_return = select(sock + 1, &fds, NULL, NULL, &timeout);
        if (select_return < 0) {   
            printf("Error with select. Closing the socket.\n");
            close(sock);
            return false;
        } else if ((select_return == 0) && (connected == true)) {
            if (received_fin == false) {
                sendAck(sock, buffer);
            }
            num_received = 0;
            timeouts++;
            if (timeouts == MAX_TIMEOUTS) {
                printf("Timed out too many times. Closing program.\n");
                close(sock);
                end_time = time(NULL);
                return true;
            }
        }
        
        if (FD_ISSET(sock, &fds)) {
            recsize = recvfrom(sock, (void*) buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &sdraddr, &slen);
        
            if (recsize <= 0) {
                printf("recvfrom failed. Closing socket. \n");
                close(sock);
            } else {
                buffer[MAX_BUFFER_SIZE] = '\0';
                
                zeroHeader();
                setHeader(buffer);
                
                if (sdr_ip == NULL) {
                    sdr_port = ntohs(sdraddr.sin_port);
                    sdr_ip   = inet_ntoa(sdraddr.sin_addr);
                }
                
                printLogMessage(1, "r");
                timeouts = 0;
                
                // If we received a SYN, send an ACK.
                if (strcmp(header.type, "SYN") == 0) {
                    ack_num = header.seq_num + 1;
                    expected_seq_num = ack_num;
                    sendAck(sock, buffer);
                    connected = true;
                    syns_recv++;
                } else if (strcmp(header.type, "DAT") == 0) {
                    t_packs++;
                    t_bytes += strlen(buffer);
                    
                    // If we received the a data packet with the next 
                    // expected sequence number, send an ACK.
                    if (header.seq_num == expected_seq_num) {
                        ack_num = header.seq_num + 1;
                        expected_seq_num = ack_num;
                        printToFile(buffer);
                        
                        num_received++;
                        u_packs++;
                        u_bytes += strlen(buffer);
                        
                        if (num_received == MAX_WINDOW_SIZE) {
                            sendAck(sock, buffer);
                            num_received = 0;
                        }
                    }
                } else if (strcmp(header.type, "FIN") == 0) {
                    received_fin = true;
                    ack_num = header.seq_num + 1;
                    expected_seq_num = ack_num;
                    sendAck(sock, buffer);
                    fins_recv++;
                }
                
            }
            
            memset(buffer, 0, MAX_BUFFER_SIZE);
        }
        
        memset(buffer, 0, MAX_BUFFER_SIZE);
    }

    end_time = time(NULL);
    return true;
}

// MAIN
int main(int argc, char* argv[]) {
    if ( !checkArguments(argc, argv) ) return 0;    
    if ( !createServer(argv) ) return 0;
    fclose(fp);
    printStats();
}

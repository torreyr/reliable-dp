// ----------------------------------------------------------------	//
//  Header file defining functions used in both rdpr.c and rdps.c. 	//
// ----------------------------------------------------------------	//

/*
 *	Validates a port number.
 */
bool isPort(char* str) {
    int portnum = atoi(str);
    if( (portnum > 0) && (portnum < 65535) ) return true;
    return false;
}

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
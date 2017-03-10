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
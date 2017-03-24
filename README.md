# reliable-dp


1. How do you design and implement your RDP header and header fields?
   Do you use any additional header fields?
   
   My header is a comma-delineated string. The magic and type fields are strings
   and the sequence number, acknowledgement number, payload size, and window size
   are integers.
   If a field is not used, it is zero.
   Only one flag is supported at a time, and it goes in the type field.

2. How do you design and implement the connection management using SYN, 
   FIN and RST packets?
   How do you choose the initial sequence number?
   
3. How do you design and implement the flow control using window size?
   How do you choose the initial window size and adjust the size?
   How do you read and write the file and manage the buffer at the 
   sender and receiver side, respectively?
   
4. How do you design and implement the error detection, notification 
   and recovery?
   How to use timer? How many timers do you use?
   How to repond to the events at the sender and receiver side?
   How to ensure reliable data transfer?
   
 
Send 10(WINDOW_SIZE) packets, drop any that are out of order, ACK the highest one we have, send the next 10.
Sequence numbers are packet numbers with an offset. Not the next byte that it's expecting. The next packet it's expecting.
Did not implement RST flags. Mostly, when things go wrong, the program exits.
Required duplicated packet event types not implemented.
For my implementation, the WINDOW_SIZE of the sender should be less than or equal to the WINDOW_SIZE of the receiver in 
order to be efficient.

RECEIVER

Every time a SYN is seen, send an ACK.
	Because the only reason another SYN will be sent is if the sender times out,
	which means they didn't get your ACK.
Whenever a timeout occurs, send an ACK.
Deals with duplicate packets by dropping anything that is not the sequence number it expects.
Unique packets are ones that arrive when it is expecting them. (they match the previous ACK number).
Therefore, unique bytes ends up being the size of the file because my implementation does not deal
with out-of-order packets.
	
SENDER

Send a SYN.
    Wait for an ACK.
    If timeout occurs and no ACK received, send another SYN with a new sequence number.
    Statistics for unique data tracking not implemented.
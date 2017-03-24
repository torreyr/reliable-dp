# reliable-dp
Torrey Randolph
V00818198
Lab Section B07


### Header
My header consists of the following fields:
```
	magic           (CSC361)
	type            (SYN/FIN/DAT/ACK/RST)
	seq_num         (sequence number)
	ack_num         (acknowledgment number)
	data_len        (payload size in bytes)
	window_size     (current window size)
```
- My header is a comma-delimited string. The magic and type fields are strings and the sequence number, acknowledgement number, payload size, and window size are integers.
- Sequence numbers increase by one each packet.
- If a field is not used, it is zero.
- Only one flag is supported at a time, and it goes in the type field. Therefore, the type field is always three characters long.

### Connection Management
Creating and closing the connection are dealt with in similar ways.
##### Creating the Connection
- The initial SYN number is random within a range of 0 to 65000.
- The sender sends a SYN packet and then waits for an ACK.
    - If it does not receive one within its timeout limit, it will resend the SYN.
    - If it times out the maximum number of times, it will give up, close the socket, and close the program.
- The receiver waits for a SYN packet.
    - It never times out so you have to manually close the program if a SYN packet is never received.
    - Once it receives a SYN, it will immediately send the corresponding ACK, then wait for data.
    - If it times out while waiting for data, it will resend the ACK.
    - If it times out the maximum number of times while waiting for data, it will close the socket and close the program.
##### Closing the Connection
- When the sender has sent the entire file, and received an ACK for the last data packet it sent, it sends a FIN packet, then waits for an ACK.
    - If it gets an incorrect ACK number, it will resend data from the corresponding point in the file.
    - If it times out, it will resend the FIN.
    - If it times out the maximum number of times, it will give up, close the socket, and close the program.
- When the receiver gets a FIN packet, it immediately sends an ACK. However, it will ACK whatever the next sequence number it is expecting is.
    - If at any point the receiver times out the maximum number of times, it will give up, close the socket, and close the program.

### Flow Control
- The sender and receiver window sizes are both set to ten packets. This was decided through optimization testing.
- The sender sends data packets in chunks of ten, then waits for an ACK. Each time a data packet is sent, the sender decrements its window size by one. When the window size gets down to zero, it is reset to ten. The receiver, after receiving ten data packets or timing out, sends an ACK.

#### File Management
- The sender reads in one packet worth of the file at a time. It calculates the necessary starting file position from the ACK number it received last.
- The receiver watches for the next expected sequence number. If it receives anything unexpected, it will drop the packet. Correctly sequenced data packets will be immediately written to file.
   
### Error Control
Stated above in __Flow Control__.
- The sender is split into three states: creating the connection, sending data, and closing the connection. Each of these states have their own timers.

### Miscellaneous
- For the receiver, unique packets are ones that arrive when it is expecting them (they match the previous ACK number). Therefore, unique bytes ends up being the size of the file because my implementation does not deal with out-of-order packets.
- Statistics for unique data tracking not implemented on the sender side.
- Duplicated packet event types not implemented.
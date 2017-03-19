# reliable-dp


1. How do you design and implement your RDP header and header fields?
   Do you use any additional header fields?

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
   
   
   
Only support one flag at a time.
Have an extra field in the header for if the packet is a resent packet.
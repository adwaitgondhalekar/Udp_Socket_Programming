This code tries to implement a reliable sliding window protocol for data transfer between client and server in C language using Unix Socket APIs. </title>
The sockets used are of UDP type.
The task performed is of reading data from a file at the client side and then transmitting that data to the server side and writing that data into another file at the server side.



Steps to be followed to run client.c and server.c on two seperate Terminals

Step 1 - Initially compile the file server.c by using command
		
	 gcc -o server server.c
	 
Step 2 - Then run the executible created by using the command
	
	 ./server
	 
	Now the server side will be running in one terminal
	
Step 3 - Now we need to compile the file client.c in another terminal by using command

	 gcc -o client -pthread client.c -lm
	 
Step 4 - Then run the executible created using the command

	 ./client
	 
	Now the client side will be running in one terminal
	
Step 5 - The client program will prompt to enter the window size that has to be used while sending packets to the server side and after that it will 

	 prompt to enter the filname present in the current folder that has to be read and sent to server side.

	 enter the filename in terminal - "file.txt"
	
	 Here any file name can be entered provided that the file is present in the current folder and is of the type ".txt"
	
	
Step 6 - As you hit enter the client program will read data from the mentioned text file and start transmitting data to the 
	server side chunk by chunk
	
	In client side terminal the packet number of sent packets and window Base will be displayed along with the packet numbers for which 
	acknowledgement has been received from server side		
	
	In the server side terminal the received packet's number and packet number for which acknowledgement was sent will be displayed along with the 
	packet number of the packets that were dropped
	
	After all the chunks are sent and acknowledgements are received on the client side the client program will end.
	
	When the server side has received all the data packets and sent acknowledgements for all of them it will write the data from 
	these packets to a file named "out.txt" one by one and then server program ends.
	
	The file "out.txt" will contain text identical to what was present in "file.txt"
	
	
NOTE - 1)Currently the program has been executed upto window size 30
	
       2)Chunk size can also be altered by changing the value of the constant named "chunksize" in both server.c and client.c
       	Currently the program has been executed on chunk sizes - 16,32 and 64
	

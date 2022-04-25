#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n (len) bytes from fd; returns true on success and false on failure. 
It may need to call the system call "read" multiple times to reach the given size len. 
*/
static bool nread(int fd, int len, uint8_t *buf) {
  //Return false if the buffer isn't big enough
  if (len > (int) sizeof(buf)) {
  	return false;
  }
  //Read bytes into buffer
  ssize_t read_check = read(fd, buf, len);
  //If read fails, return false
  if (read_check == -1) {
  	return false;
  }
  return true;
  
}

/* attempts to write n bytes to fd; returns true on success and false on failure 
It may need to call the system call "write" multiple times to reach the size len.
*/
static bool nwrite(int fd, int len, uint8_t *buf) {
  //Return false if the buffer isn't big enough
  if (len > (int) sizeof(buf)) {
  	return false;
  }
  //Read bytes into buffer
  ssize_t write_check = write(fd, buf, len);
  //If read fails, return false
  if (write_check == -1) {
  	return false;
  }
  return true;
  
}

/* Through this function call the client attempts to receive a packet from sd 
(i.e., receiving a response from the server.). It happens after the client previously 
forwarded a jbod operation call via a request message to the server.  
It returns true on success and false on failure. 
The values of the parameters (including op, ret, block) will be returned to the caller of this function: 

op - the address to store the jbod "opcode"  
ret - the address to store the return value of the server side calling the corresponding jbod_operation function.
block - holds the received block content if existing (e.g., when the op command is JBOD_READ_BLOCK)

In your implementation, you can read the packet header first (i.e., read HEADER_LEN bytes first), 
and then use the length field in the header to determine whether it is needed to read 
a block of data from the server. You may use the above nread function here.  
*/
static bool recv_packet(int sd, uint32_t *op, uint16_t *ret, uint8_t *block) {
}



/* The client attempts to send a jbod request packet to sd (i.e., the server socket here); 
returns true on success and false on failure. 

op - the opcode. 
block- when the command is JBOD_WRITE_BLOCK, the block will contain data to write to the server jbod system;
otherwise it is NULL.

The above information (when applicable) has to be wrapped into a jbod request packet (format specified in readme).
You may call the above nwrite function to do the actual sending.  
*/
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
}



/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. 
 * this function will be invoked by tester to connect to the server at given ip and port.
 * you will not call it in mdadm.c
*/
bool jbod_connect(const char *ip, uint16_t port) {

	// Set up the socket and its properties; port and IP are already passed in and socket descriptor is global so we don't need to set those
	struct sockaddr_in caddr;
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(port);

	//Sending errors
	if (inet_aton(ip, &caddr.sin_addr) == 0) {
		return(false);
	}
	//Create socket, return false if connects
	cli_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (cli_sd == -1) {
		printf("Error on socket creation [%s]\n", strerror(errno));
		return(false);
	}
	//Connect to socket, return false if fails
	if (connect(cli_sd, (const struct sockaddr *)&caddr, sizeof(caddr) == -1)) {
		printf("Error on socket connect [%s]\n", strerror(errno));
		return(false);
	}

	return true;
	
}




/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
	close(cli_sd); //Close socket
	cli_sd = -1; //reset value of cli_sd
}



/* sends the JBOD operation to the server (use the send_packet function) and receives 
(use the recv_packet function) and processes the response. 

The meaning of each parameter is the same as in the original jbod_operation function. 
return: 0 means success, -1 means failure.
*/
int jbod_client_operation(uint32_t op, uint8_t *block) {
}

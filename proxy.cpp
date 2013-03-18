/* This file contains the main method for the proxy- server.
 * Making use of the utility header file socket_util.h & http_header_util.h, it does the following :
 	1.  Creates a Socket and binds it to a port --> specified as cmd-line argument.
	2.  Starts listening on the socket.
	3.  Accept connect from the client.
	4.  Receive HTTP request from the Client.
	5.  Parses/Tokenizes the HTTP Header and Checks for errors & unsupported methods. If Error in request, send the error to client.
	6.  Extracts the hostname from HTTP Header and establishes a connection over a new socket with the Web-server. 
	7.  Makes a HTTP request to the server, the HTTP Packet being in Relative path format.
	8.  Receive HTTP Response from web-server.
	9.  Forward the Response to the client. 
	10. Close the connection with client. (Web-server automatically closes connection)
	11. Goto Step 3.
*/
#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include "http_header_util.h"
#include "socket_util.h"

#define MAX_MESSAGE_SIZE 100000
#define BACKLOG 10

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2) {		//Must supply correct number of arguments
		fprintf(stderr,"usage: ./proxy <port>\n");
		return 1;
	}
	/* === 1. Bind a socket over the specified port === */
	int sockfd = getBindStremeSocket(argv[1]);		
	if (sockfd == -1)				//error - Unable to bind a socket over the port
		return 1;				
	/* === 2. Start Listening on the bound socket ===*/
	if (listen(sockfd, BACKLOG) == -1) {		
		perror("listen");			//error - Unable to listen on the socket	
		exit(1);
	}
	printf("Proxy-Server: waiting for connections on port:%s ...\n", argv[1]);	//Server Listening on Port argv[1]
	
	
	while(1){
		char buf[MAX_MESSAGE_SIZE] = {};
		
		/* === 3. Accept Connection from the Client === */
		struct sockaddr_storage client_addr;
	       	socklen_t sin_size = sizeof client_addr;
		int newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
		if (newfd == -1) {
			perror("accept");
			continue;
		}
		
		char s[INET6_ADDRSTRLEN];
		get_client_info(&client_addr, s);
		printf("Proxy-Server: got connection from %s\n",s);
		
		
		/* === 4. Receive HTTP Request from the Client === */
		
		//Keep receiving over the connection untill the message ends with two consecutive CRLF.
		int i, len = 0, count = 0;
		for (int len_receive = sizeof buf; (i = recv(newfd, &buf[len], len_receive, 0)) != -1 && count < 100 ;count++){
		       	len += i;
			if(buf[len-1] == '\n'){
				if (buf[len-2] == '\n' || (len >= 3 && buf[len-3] == '\n' && buf[len-2] == '\r'))
					break;
			}
		}
	
		//Check for Receive Error
		if (i == -1){				
			perror("recv");
			close(newfd);
			continue;
		}
		
		//No Data sent in 100 consecutive receives after connection was made (this case arises when request from browsers)
		if (count >= 100){
			printf("Timeout (at proxy) ....\n");
			continue;
		}
		
		buf[len] = '\0';			//Add Null terminator after the message
		printf("Request From Client :-\n%s\n", buf);


		/* === 5. Parse the received request and check for errors. If error exits, send mesg to client === */
		
		//Make a HTTPHeader Object for the received request. Atomatically parse and generates errorno.
		HTTPHeader client_packet(buf);				
		
		//Get the error no. If error no is 0, no error
		int errno;
		if ((errno = client_packet.getErrNo()) != 0){	//There is error in the HTTP request from client
			HTTPHeader::getErrMsg(errno, buf);
			fprintf(stderr, "Error in Request(Msg Sent to client) : %s\n", buf);
				
			//Send the error message to client
			int len_sent = strlen(buf);
			buf[len_sent] = '\r';
			buf[len_sent+1] = '\n';
			buf[len_sent+2] = '\0';
			if (send(newfd, buf, len_sent+3, 0) == -1)
				perror("send");
			close(newfd);
			continue;
		}

		/* === 6. Extracts the hostname from HTTP request and establishes a connection over a new socket with the Web-server === */
		
		//Get a connect stream socket to the Web-server (Take Host name and port from HTTPHeader object)
		int connect_sockfd = getConnectStremeSocket(client_packet.getHostName(), client_packet.getPort());
		if (connect_sockfd == -1){		//error in acquiring a connection to web-server
			close(newfd);
			close(connect_sockfd);
			continue;
		}
		printf("Connected to Web-server: %s on port: %s\n\n", client_packet.getHostName(), client_packet.getPort());


		/* === 7. Construct a HTTP request(Relative path format) and send the request to the Web-server === */
		
		//make a new packet for to be sent to the server
		client_packet.make_header(buf);

		//send the request to the server
		int len_sent = strlen(buf);
		if ( send(connect_sockfd, buf, len_sent, 0) == -1){
			perror("send");			//Error in sending request to server
			close(newfd);
			continue;
		}
		printf("Request Sent To Web-Server :-\n%s\n",buf);
		
		
		/* === 8. Receive HTTP Response from web-server === */
		
		//response received from Web-server
		int len_receive = sizeof buf;
		if ( (len = recv(connect_sockfd, buf, len_receive, 0)) == -1){
			perror("recv");		//receive error
			close(newfd);
			continue;
		}
		buf[len] = '\0';		//terminate buffer(message received) with null character
		printf("Response Received From Web-Server :-\n%s\n",buf);

		/* === 9.  Forward Response to the client === */
		
		// send response to client
		len_sent = strlen(buf);
		if (send(newfd, buf, len_sent, 0) == -1){
			perror("send");		//send error
			close(newfd);
			continue;
		}
		printf("Response to Client :-\n%s\n",buf);

		/* === 10. Close the Connection with the client === */
		close(newfd);
	}

	return 0;
}

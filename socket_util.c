/* socket_util.c --> This file is the collection of utility functions which have been used in the proxy.cpp file to DEAL WITH SOCKETS. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include "socket_util.h"		//Header file containg function declarations for this file

/* This function accepts a port as an argument and binds a stream socket to that port, returning the socket file descripter.
 *  If this cannot be done it returns -1 and prints a usefule error message at the stderr */
int getBindStremeSocket(char *port){
	
	struct addrinfo hints;				//hints --> hints given to getaddrinfo() to get host address information.
	struct addrinfo *proxyinfo, *p;			//proxyinfo --> address information for proxy-server(localhost).	
	int sockfd;					//socketfd --> The socket file descripter which is bound to the port.
	int status;					//status --> stores the status of getaddrinfo() success or error.
	int yes = 1;					//yes --> used by setsockopt() to set a socket option. 

	/* Setting up hints passed to getaddrinfo() for getting address information */
	memset(&hints, 0, sizeof hints);		//Initialize hints to all zero.
	hints.ai_family = AF_UNSPEC; 			//Address family - here unspecified/any. Use AF_INET (IPv4) & AF_INET6 (IPv6) to force. 
	hints.ai_socktype = SOCK_STREAM;		//Socket Type - here Stream Socket (Used for TCP protocol).
	hints.ai_protocol = IPPROTO_TCP;		//Protocol Type - here TCP protocol 
	hints.ai_flags = AI_PASSIVE;			//The socket address will be used in a call to bind() function. So self-address.

	/* Getting the addressinfo structure list for proxy-server.
	 * getaddrinfo() is fed with hostname, port, hints using which it fill the proxyinfo structure list.
	 * getaddrinfo() returns 0 on sucess, otherwise error number - which we stored in status(used to print the error).*/
	if ((status = getaddrinfo(NULL, port, &hints, &proxyinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	/* Loopping through all the results (different possible addresses(with ports) for proxy-server) and bind to the first we can. */
	for(p = proxyinfo; p != NULL; p = p->ai_next) {
		/* Creates a socket with earlier specified type and protocol */
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("proxy-server: socket");	//Print error to stderr with number.
			continue;
		}

		/* Sets socket option to allows the socket to be bound to an address that is already in use.
		 * Helps in faster reconneting without waiting for previous socket to relinquish the port. */
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");		//Print error to stderr with number.
			exit(1);			
		}

		/* Binds the created socket to a particular address (IP and ports) at localhost */
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);			//if error, close the created socket.
			perror("proxy-server: bind");	//Print error to stderr with number.
			continue;			
		}

		break;					//As soon as you successfully bind a socket, break out
	}
	if (p == NULL)  {				//Not able to create and bind any socket
		fprintf(stderr, "proxy-server: failed to bind\n");
		return -1;
	}

	freeaddrinfo(proxyinfo); 			//all done with this structure (freeaddrinfo --> frees addressinfo structure)

	return sockfd;					//return the socket file descriptor
}



/* This function accepts the hostname and port for a host and establishes a connecion over a stream socket to that host.
 * If successfully connected, returns the socket file descripter.
 * If an error occurs, returns -1 and prints a usefule error message at the stderr */
int getConnectStremeSocket(char *hostname, char *port){
	struct addrinfo hints;				//hints --> hints given to getaddrinfo() to get remote host address information.
	struct addrinfo *servinfo, *p;			//servinfo --> address information for Web-server to connect to.	
	int sockfd;					//socketfd --> The socket file descripter over which we connect.
	int status;					//status --> stores the status of getaddrinfo() success or error.

	/* Setting up hints passed to getaddrinfo() for getting address information */
	memset(&hints, 0, sizeof hints);		//Initialize hints to all zero.
	hints.ai_family = AF_UNSPEC; 			//Address family - here unspecified/any. Use AF_INET (IPv4) & AF_INET6 (IPv6) to force.
	hints.ai_socktype = SOCK_STREAM;		//Socket Type - here Stream Socket (Used for TCP protocol).
	hints.ai_protocol = IPPROTO_TCP;		//Protocol Type - here TCP protocol 

	/* Getting the addressinfo structure list for Web-server (Internally does DNS Hostname Resolution).
	 * getaddrinfo() is fed with hostname, port, hints using which it fill the servinfo structure list.
	 * getaddrinfo() returns 0 on sucess, otherwise error number - which we stored in status(used to print the error).*/
	if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	/* Loopping through all the results (different possible addresses(with ports) for Web-server) and Connect to the first we can. */
	for(p = servinfo; p != NULL; p = p->ai_next) {
		/* Creates a socket with earlier specified type and protocol */
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("proxy-client: socket");
			continue;
		}

		/* Note that: here we donot set socket option for reusing a port as this is managed by OS */

		/* Connects over the created socket to the Web-server on the extarcted IP. */
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("proxy-client: connect");
			continue;
		}
		
		break;					//As soon as you successfully connect over a socket, break out.

	}

	if (p == NULL)  {				//Not able to connect over any socket
		fprintf(stderr, "proxy-client: failed to connect\n");
		return -1;
	}
	
	freeaddrinfo(servinfo); 			//all done with this structure (freeaddrinfo --> frees addressinfo structure)

	return sockfd;					//return the socket file descriptor
}

/* get sockaddr, for any the type IPv4 or IPv6 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Gets the presentable/printable ip address (char from binary) available in the sockaddr_storage structure and puts it into s 
 * inet_ntop --> converts address from network friendly form to presentable/print friendly form */
void get_client_info(struct sockaddr_storage *client_addr, char s[INET6_ADDRSTRLEN]){
	inet_ntop(client_addr -> ss_family, get_in_addr((struct sockaddr *)client_addr), s, INET6_ADDRSTRLEN);
}

/* Useful debugging function, not used anywhere in the code.
 * Prints the address in the derieved addressinfo struncture in a presentable form. */
void printTypeAndIP(struct addrinfo *res){
	printf("Printing IP addresses from list : \n\n");
	for(struct addrinfo *p = res; p != NULL; p = p->ai_next) {
		void *addr;
		char ipver[6];
		char ipstr[INET6_ADDRSTRLEN];
		//get the pointer to the address itself, different fields in IPv4 and IPv6:
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			strcpy(ipver,"IPv4");
		} else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			strcpy(ipver,"IPv6");
		}

		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf(" %s: %s\n", ipver, ipstr);
	}
}

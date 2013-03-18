/* socket_util.h --> Header file for socket_util.c. Contains Declaration of various utility functions for SOCKETS used in proxy.cpp. */

#ifndef SOCKET_GRAB_H
#define SOCKET_GRAB_H

/* This function accepts a port as an argument and binds a stream socket to that port, returning the socket file descripter.
 * If this cannot be done it returns -1 and prints a usefule error message at the stderr */
int getBindStremeSocket(char *port);

/* This function accepts the hostname and port for a host and establishes a connecion over a stream socket to that host.
 * If successfully connected, returns the socket file descripter.
 * If an error occurs, returns -1 and prints a usefule error message at the stderr */
int getConnectStremeSocket(char *hostname, char *port);

/* get sockaddr, for any the type IPv4 or IPv6 */
void *get_in_addr(struct sockaddr *sa);

/* Gets the presentable/printable ip address (char from binary) available in the sockaddr_storage structure and puts it into s 
 * inet_ntop --> converts address from network friendly form to presentable/print friendly form */
void get_client_info(struct sockaddr_storage *client_addr, char* s);

/* Useful debugging function, not used anywhere in the code.
 * Prints the address in the derieved addressinfo struncture in a presentable form. */
void printTypeAndIP(struct addrinfo *res);

#endif

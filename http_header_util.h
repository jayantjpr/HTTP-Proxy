/* http_header_util.h --> This files declares the class HTTPHeader which is used by proxy.cpp to work with HTTP Header recieved from client.
 * The main purpose of the HTTPHeader class is to parse & tokenize the HTTP header received from client and store these tokens. 
 * This helps in multiple way :
 	* To reliably extract the url from the HTTP Header.
 	* To Validitate the HTTP Header (check its correctness) and generate respective errors.
 	* Construct a new header in a consistent manner.
 */

#ifndef HEADER_H
#define HEADER_H

#include <vector>
#include <utility>

class HTTPHeader
{
	char *hostname;						//The hostname of the web-server in client packet.
	char *port;						//The port of web-server to which the client wants to connect.
	char *relative_addr;					//The relative path of the resource on the server required by client.
	int err_no;						//The error number generated while parsing the Header and URL
	std::vector < std::pair<char*, char*> > headers;	//The collection of <Header Name, Value> pairs

	public :

	/* Constructor for HTTPHeader Class objects*/
	HTTPHeader(char* buf);

	/* Getters */
	char* getHostName();					//Getter for hostname
	char* getPort();					//Getter for port
	int getErrNo();						//Getter for err_no
	
	/* Get the error message for the specified error number and fill the errmsg*/
	static void getErrMsg(int errno, char errmsg[]);

	/* Parse(tokenize) the HTTP header and store the token. Simultaneously check for errors in the HTTP header */
	int parseHTTPHeader(char *buf);

	/* This function parse the HTTP URL and extracts the hostname, port and relative address. Simultaneously check for errors in the HTTP URL */
	int parseURL(char *line);

	/* This Function makes the HTTP Header in the RELATIVE format from the available tokens */
	void make_header(char *buf);

	/* Destructor for HTTPHeader Class objects*/
	~HTTPHeader();
};	

#endif

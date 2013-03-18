/* http_header_util.cpp --> This files defines the member functions for the class HTTPHeader.
 * The main purpose of the HTTPHeader class is to parse & tokenize the HTTP header received from client and store these tokens. 
 * This helps in multiple way :
 	* To reliably extract the url from the HTTP Header.
 	* To Validitate the HTTP Header (check its correctness) and generate respective errors.
 	* Construct a new header in a consistent manner.
 */

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utility>
#include <vector>
#include "http_header_util.h"

#define RELATIVE 0
#define ABSOLUTE 1

using namespace std;

/* Constructor for HTTPHeader Class objects*/
HTTPHeader::HTTPHeader(char* buf){
	hostname = new char[1]();			//Initializing HostName to '\0'
	port= new char[6];
	strcpy(port, "80");				//Initializing port to 80
	relative_addr = new char[1]();			//Initializing relative_addr to '\0'

	err_no = parseHTTPHeader(buf);			//Parse the header and put the error number in err_no(if no error parseHTTPHeader returns 0)
}

/* Destructor for HTTPHeader Class objects - frees memory allocated over heap*/
HTTPHeader::~HTTPHeader(){
	for (vector< pair<char*, char*> >::iterator it = headers.begin(); it != headers.end(); it++){
		free(it -> first);
		free(it-> second);
	}
	free(hostname);
	free(port);
	free(relative_addr);
}

/* Getter for hostname */
char*  HTTPHeader:: getHostName(){
	return hostname;
}

/* Getter for port */
char* HTTPHeader::getPort(){
	return port;
}

/* Getter for err_no */
int HTTPHeader::getErrNo(){
	return err_no;
}

/* Get the error message for the specified error number and fill the errmsg*/
void HTTPHeader::getErrMsg(int errno, char errmsg[]){
	switch(errno){
		case 0:
			strcpy(errmsg,"");
			break;
		case 400:
			strcpy(errmsg,"400 Bad Request");
			break;
		case 501:
			strcpy(errmsg,"501 Not Implemented");
			break;
	}
}

/* Parse(tokenize) the HTTP header and store the token. Simultaneously check for errors in the HTTP header */
int HTTPHeader::parseHTTPHeader(char *buf){

	//Check --> HTTP Header ends in CRLF
	if ((strstr(buf, "\n\r\n") == NULL))
		return (400);

	//line-by-line Tokenization
		char *line, *save_ptr1;

	/* Check --> Format of the first line (<Method> <URL Path> <Version>)
	 * Extract --> (for both)Relative Address, (for Absolute format) Hostname & port */
	int path_url = ABSOLUTE; 
	{
		if (!(line = strtok_r(buf, "\r\n", &save_ptr1)))
			return 400;
		char *word, *save_ptr2;

		//Check <Method> - Must be GET
		{
			word = strtok_r(line, " ", &save_ptr2);
			if (strcmp(word,"GET"))
				return(501);				//Methods other than GET not implemented
		}

		//Check <URL Path> - RELATIVE or ABSOLUTE
		{
			if (!(word = strtok_r(NULL, " ", &save_ptr2)))
				return 400;
			if (word[0] == '/'){				//Relative Address format of <url path>
				free(relative_addr);
				relative_addr = new char[strlen(word)+1];
				strcpy(relative_addr ,&word[1]);
				path_url = RELATIVE;
			}
			else if (parseURL(word))			//Absolute Address Format of <url path> --> parse the URL
				return 400;
		}

		//Check <Version> - HTTP/1.0 (or) HTTP/1.1
		{
			if (!(word = strtok_r(NULL, " ", &save_ptr2)))
				return 400;
			if (strcmp(word,"HTTP/1.0") && strcmp(word,"HTTP/1.1"))
				return 400;
		}
	}

	/* Check --> Host Header
	 * Extract --> hostname, port (in case of Relative format) */
	{
		if (!(line = strtok_r(NULL, "\r\n", &save_ptr1))){
			if (path_url == RELATIVE)
				return 400;
			if (path_url == ABSOLUTE)
				return 0;
		}
		if ( strchr(line, ':') == NULL || (strchr(line, ' ')-line) < (strchr(line, ':')-line) )
			return 400;
		char *word, *save_ptr2;

		
		
		if (path_url == RELATIVE){

			//Check - Existense of Host Header
			{
				word = strtok_r(line, ":", &save_ptr2);
				if (strcmp(word, "Host"))
					return 400;
			}

			//Parse the hostname(URL)
			{
				if (!(word = strtok_r(NULL, "\r\n", &save_ptr2)))
					return 400;
				if (strstr(word, "http://")){
					if (parseURL(word+1))
						return 400;
				}
				else{
					char *hostname = new char[strlen("http://")+strlen(word)+1];
					strcpy(hostname, "http://");
					if (parseURL(strcat(hostname,word+1)))
						return 400;
				}
			}
		}
		else{
			word = strtok_r(line, ":", &save_ptr2);
			if (strcmp(word, "Host")){		//Host Header may not necessirally exist
				char *header_name = new char[strlen(word)+1]; 
				strcpy(header_name,word);

				if (!(word = strtok_r(NULL, "\r\n", &save_ptr2)))
					return 400;
				char *value = new char[strlen(word)]; 
				strcpy(value,word+1);

				headers.push_back(make_pair(header_name,value));	//Push <Header Name, Value> pair in Vector headers	
			}
		}
	}	

	/* For the remaining lines, extract the <Header Name, Value> pair */
	while ( (line = strtok_r(NULL, "\r\n", &save_ptr1)) ){
		if ( strchr(line, ':') == NULL || (strchr(line, ' ')-line) < (strchr(line, ':')-line) )
			return (400);
		char *word, *save_ptr2, *header_name, *value;

		//Extract --> Header Name
		{
			word = strtok_r(line, ":", &save_ptr2);
			header_name = new char[strlen(word)+1]; 
			strcpy(header_name,word);
		}
		
		//Extract --> Header Value
		{
			if (!(word = strtok_r(NULL, "\r\n", &save_ptr2)))
				return 400;
			value = new char[strlen(word)]; 
			strcpy(value,word+1);
		}
		
		headers.push_back(make_pair(header_name, value));	//Push <Header Name, Value> pair in Vector headers
	}

	return 0;
}


/* This function parse the HTTP URL and extracts the hostname, port and relative address. Simultaneously check for errors in the HTTP URL */
int HTTPHeader::parseURL(char *line){

	if (strchr(line, ' '))			//HTTP URLs donot consist of spaces
		return 400;
	char *save_ptr1;
	
	//Check - http protocol declarartion
	{
		char *word;
		if ( !(word = strtok_r(line, "/:", &save_ptr1)) )
			return 400;
		if (strcmp(word, "http"))
			return 400;
	}

	//Check and Extract - hostname & port
	{
		char *word, *sub_word, *save_ptr2;
		if ( !(word = strtok_r(NULL, "/", &save_ptr1)) )
			return 400;

		if (strchr(word, ':')){
			//hostname-check & extarct
			if ( !(sub_word = strtok_r(word, ":", &save_ptr2)) )
				return 400;
			free(hostname);
			hostname = new char[strlen(sub_word)+1];
			strcpy(hostname, sub_word);

			//port-check & extarct
			if ( !(sub_word = strtok_r(NULL, ":", &save_ptr2)) )
				return 400;
			strcpy(port, sub_word);
		}
		else{
			//hostname-check & extarct
			if ( !(sub_word = strtok_r(word, ":", &save_ptr2)) )
				return 400;
			free(hostname);
			hostname = new char[strlen(sub_word)+1];
			strcpy(hostname, sub_word);
		}

	}

	//Check & Extract - Relative Path (if at all it exists)
	{
		char *word;
		if ( (word = strtok_r(NULL, "/", &save_ptr1)) ){
			free(relative_addr);
			relative_addr = new char[strlen(word)+1];
			strcpy(relative_addr,word);
		}
	}

	return 0;
}


/* This Function makes the HTTP Header in the RELATIVE format from the available tokens */ 
void HTTPHeader::make_header(char *buf){
	int pos = 0;
	pos += sprintf(buf, "GET /%s HTTP/1.0\r\n", relative_addr);
	if (strcmp(port,"80"))
		pos += sprintf(&buf[pos], "Host: %s:%s\r\n", hostname,port);
	else
		pos += sprintf(&buf[pos], "Host: %s\r\n", hostname);


	for (vector< pair<char*, char*> >::iterator it = headers.begin(); it != headers.end(); it++)
		pos += sprintf(&buf[pos], "%s: %s\r\n", it->first, it->second);

	sprintf(&buf[pos], "\r\n");
}


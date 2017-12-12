#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define __MAC_OS_X

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


//1
#define CONNQSIZE 16
#define LOGQSIZE 16
#define NTHREADS 5
int_sbuf_t connQ;
str_sbuf_t logQ;
sem_t mutex;
cNode *head = NULL;
//1


/* $begin tinymain */
/*
* tiny.c - A simple, iterative HTTP/1.0 Web server that uses the GET method
* to serve static and dynamic content.
*/

void* doit(void* arg);
int doRequest(rio_t *rio, char *uri);
void doResponse(int cfd, int fd, char *uri);

void read_requesthdrs(rio_t * rp);
int	parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
	char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
	int	listenfd, connfd;
	char hostname [MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;

	/* Check command line args */
	if (argc != 2){
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

//1
	Sem_init(&mutex, 0, 1);
//1

	listenfd = Open_listenfd(argv[1]); //open and return a listening socket on the specified port. This function is reentrant and protocol-independent.

//1
	int_sbuf_init(&connQ, CONNQSIZE);
	str_sbuf_init(&logQ, LOGQSIZE);
//1


	while (1){
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *) & clientaddr, &clientlen);

//1
		int_sbuf_insert(&connQ, connfd);
//1

		//line: netp: tiny:accept
		Getnameinfo((SA *) & clientaddr, clientlen, hostname, MAXLINE,
			port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		//doit(connfd);
		//line: netp: tiny:doit
		pthread_t tid; //Thread id

		int* arg = malloc(sizeof(int));

		*arg = connfd;

		Pthread_create(&tid, NULL, doit, arg); //new thread with id tid, default values, calling doit, with the arg of connfd
		
	}
}
/* $end tinymain */

/*
* doit - handle one HTTP request/response transaction
*/
/* $begin doit */
void* doit(void* arg){

	int fd = *((int*)arg);

	free(arg);

	Pthread_detach(pthread_self());

	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	//char filename [MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

//1
	fd = int_sbuf_remove(&connQ); //removes one of the connections from our buffer
//1

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);

	if (!Rio_readlineb(&rio, buf, MAXLINE))
		
		return NULL;
		printf("%s", buf);

		sscanf(buf, "%s %s %s", method, uri, version);			
	
	if (strcasecmp(method, "GET")){								
		printf("Error, not yet implemented\n");
	}
	else{

//1
		P(&mutex);
		cNode *cached_item = find(head, uri);
		V(&mutex);

		if(cached_item != NULL){
			P(&mutex);
			mostRecent(head, cached_item);
			V(&mutex);

			//safe send shtuff;
			//safe_send(fd, cached_item->bytes, cached_item->nbytes);
			rio_writen(fd, cached_item->content, cached_item->size);


		}
		else{
			int cfd = doRequest(&rio, uri);
			doResponse(cfd, fd, uri);

		}

		str_sbuf_insert(&logQ, "Proxy request processed\n");
//1		
		
		
		Close(cfd);
		Close(fd);
		return NULL;
	}

	Close(fd);

	str_sbuf_insert(&logQ, "Normal (non-proxy) request processed\n");

	return NULL;
}
/* $end doit */


//reads the entirety of the request from client, parses the request into host, port, and uri. 
//  Determines if request is valid, then establishes a connection to the web server and carries out the request.
int doRequest(rio_t *rio, char *uri){
	
	char host[MAXLINE], port[MAXLINE], newHost[MAXLINE], hostPath[MAXLINE];;
	const char *ptr = strchr(uri, ':');
	
	int hostIndex = ptr - uri;
   hostIndex = hostIndex + 3;
   
   const char *uriCopy = uri;
   int uriLength = strlen(uri);
   int count;
   for(count = 0; count < 3 ; count++){
       uriCopy = strstr(uriCopy, "/");
       uriCopy++;
   }

   int endHostIndex = (uriCopy - uri) - 1;
   strncpy(host, uri+hostIndex, endHostIndex - hostIndex);
   const char *colonPtr = strchr(host, ':');
   
   //this means port is specified, clip that out of host and clean everything up into host and port.
   if(colonPtr != NULL){

       int colonIndex = colonPtr - host;   
       int hostLength = colonIndex;
       int portLength = strlen(host)-(colonIndex+1);

       strncpy(port, host+colonIndex+1, portLength);
       int portLen = strlen(port);
       port[portLen] = '\0';

       strncpy(newHost, host, hostLength);
       int hostLen = strlen(newHost);
       newHost[hostLen] = '\0';
   }
   else{
       
       strncpy(newHost, host, strlen(host));
       int hostLen = strlen(newHost);
       newHost[hostLen] = '\0';
       strcpy(port, "80");
   }

   strncpy(hostPath, uri+endHostIndex, uriLength-endHostIndex);

   strcpy(host, newHost);

	//I have host number and port stuff out of uri, now send to openclientfd and get teh file descriptor we need
	int cfd = open_clientfd(host, port);

	//Build and get line.
	char request[MAXLINE];
	sprintf(request, "GET %s HTTP/1.1\r\n", hostPath);
	
	//send the rest of the request:
	sprintf(request, "%sHost: %s\r\n", request, host);
	sprintf(request, "%sUser-Agent: %s\r\n", request, user_agent_hdr);
	sprintf(request, "%sProxy-Connection: %s\r\n", request, "Close");
	sprintf(request, "%sConnection: %s\r\n", request, "Close");
	sprintf(request, "%s\r\n", request);

	rio_writen(cfd, request, strlen(request));

	return cfd;
}

void doResponse(int cfd, int fd, char *uri){

	char response[MAXLINE];
	rio_t rServer;
	int bodyLength;
	int lineLength;
	//char buffer[MAXLINE];

//1
	char fullBuffer[MAXLINE];
	int fullBufferSize;
	int reponsePlaceholder;
//1

	int messageLength;
	
	//Responding to client, first find the length of the message, then send that much back to client
	Rio_readinitb(&rServer, cfd);
	
	while(strlen(response) > 0 && strstr(response,"\r\n") != 0){
		
		//get line length to use later when writing our response
		lineLength = Rio_readlineb(&rServer, response, MAXLINE);

//1
		fullBufferSize = lineLength + fullBufferSize;
//1
      
      //grab the response body length to use later in our body response
      if(strstr(response, "Content-length:")){
         bodyLength = atoi(response + strlen("Content-length:"));

//1
         fullBufferSize = bodyLength + fullBufferSize;
//1

      }
		

		//writing response so far, will write again with whole body
//1   
      memcpy(completeResponse[reponsePlaceholder], response, lineLength);
      reponsePlaceholder +=lineLength;
      rio_writen(fd, response, lineLength);
      //rio_writen(fd, response, lineLength);
//1
      
      
	}

	//writing the body response here
   while(messageLength < bodyLength){
		
      lineLength = Rio_readlineb(&rServer, response, MAXLINE);
      rio_writen(fd, response, lineLength);
      
	  messageLength = messageLength + lineLength;
	  memcpy(completeResponse[reponsePlaceholder], response, lineLength);
	  rio_writen(fd, response, lineLength);
   }

//1
   if(fullBufferSize < MAX_OBJECT_SIZE){
   		P(&mutex);
   		head = insert(head, uri, fullBufferSize, fullBuffer);
   		V(&mutex);
   }
//1

	

}

//1
void *logger(void *vargp){
	Pthread_detach(pthread_self());
	FILE *fp;
	fp = fopen("/tmp/jameslogger.txt", "a");
	while(1){
		char *msg = str_sbuf_remove(&logQ);
		fprintf(fp,msg);
	}

	fclose(fp);

}

//1



/*
* read_requesthdrs - read HTTP request headers
*/
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t * rp){

	char buf[MAXLINE];
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);

	while (strcmp(buf, "\r\n")){		//line: netp: readhdrs:checkterm
		
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}
/* $end read_requesthdrs */

/*
* parse_uri - parse URI into filename and CGI args return 0 if dynamic
* content, 1 if static
*/
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs){
	
	char *ptr;

	if (!strstr(uri, "cgi-bin")){			/* Static content */	//line: netp: parseuri:isstatic
		
		strcpy(cgiargs, "");					//line: netp: parseuri:clearcgi
		strcpy(filename, ".");					//line: netp: parseuri:beginconvert1
		strcat(filename, uri);					//line: netp: parseuri:endconvert1
		if (uri[strlen(uri) - 1] == '/')		//line: netp: parseuri:slashcheck
			strcat(filename, "home.html");		//line: netp: parseuri:appenddefault
		return 1;
	}
	else{			/* Dynamic content */		//line: netp: parseuri:isdynamic
		ptr = index(uri, '?');					//line: netp: parseuri:beginextract
	if (ptr){
		strcpy(cgiargs, ptr + 1);
		*ptr = '\0';
	}
	else
		strcpy(cgiargs, "");					//line: netp: parseuri:endextract
		strcpy(filename, ".");					//line: netp: parseuri:beginconvert2
		strcat(filename, uri);					//line: netp: parseuri:endconvert2
		return 0;
	}
}
/* $end parse_uri */

/*
* serve_static - copy a file back to the client
*/
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize){

	int	srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

/* Send response headers to client */
	get_filetype(filename, filetype);		//line: netp: servestatic:getfiletype
	sprintf(buf, "HTTP/1.0 200 OK\r\n");	//line: netp: servestatic:beginserve
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));		//line: netp: servestatic:endserve
	printf("Response headers:\n");
	printf("%s", buf);

/* Send response body to client */
	srcfd = Open(filename, O_RDONLY, 0);	//line: netp: servestatic:open
	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);		//line: netp: servestatic:mmap
	Close(srcfd);							//line: netp: servestatic:close
	Rio_writen(fd, srcp, filesize);			//line: netp: servestatic:write
	Munmap(srcp, filesize);					//line: netp: servestatic:munmap
}

/*
* get_filetype - derive file type from file name
*/
void get_filetype(char *filename, char *filetype){

	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else
		strcpy(filetype, "text/plain");
}
/* $end serve_static */

/*
* serve_dynamic - run a CGI program on behalf of the client
*/
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs){

	char buf[MAXLINE], *emptylist[] = {NULL};

/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if (Fork() == 0){			/* Child */								//line: netp: servedynamic:fork
	/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1);								//line: netp: servedynamic:setenv
		Dup2(fd, STDOUT_FILENO);	/* Redirect stdout to client */		//line: netp: servedynamic:dup2
		Execve(filename, emptylist, environ);	/* Run CGI program */	//line: netp: servedynamic:execve
	}

	Wait(NULL);		/* Parent waits for and reaps child */				//line: netp: servedynamic:wait
}
/* $end serve_dynamic */

/*
* clienterror - returns an error message to the client
*/
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){

	char buf[MAXLINE], body[MAXBUF];

/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=" "ffffff" ">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */

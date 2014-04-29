/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"
#include <stdio.h>
#include <string.h>

#define HTTPVN = "HTTP/1.0\r\n"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char* buf, char* host, char* uri, char* version,
                                     char* portnm, char* htmlfile);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg);

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connect_hdr = "Connection: close";
static const char *proxy_connect_hdr = "Proxy-Connection: close";

int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port); //error handle

    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
	doit(connfd); //to be threaded
	Close(connfd);
    }
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd)
{
    //detach thread
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    //extra buf here
    char serverBuf[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    char host[MAXLINE];
    char portnm[MAXLINE], htmlfile[MAXLINE];
    rio_t rio;
    rio_t rhdr;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    printf("\nnew request below:\n");
    printf(buf);
    fflush(stdout);

    //buf, host, uri, version
    is_static = parse_uri(buf, host, uri, version, portnm, htmlfile);
    int portn = 80;
    if(strlen(portnm) != 0)
        portn = atoi(portnm);
    int hostn = open_clientfd(host, portn);

    Rio_readinitb(&rhdr, hostn); //idk?


    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
      int clientfd = Open_clientfd_r(host, portn);
/*      Rio_writen(clientfd, request from user, string length of request);

        Rio_writen(clientfd, , strlen());
*/
      Rio_writen(clientfd, (void*)user_agent_hdr, strlen(user_agent_hdr));
      Rio_writen(clientfd, (void*)accept_hdr, strlen(accept_hdr));
      Rio_writen(clientfd, (void*)accept_encoding_hdr,
                 strlen(accept_encoding_hdr));

      Rio_writen(clientfd, (void*)connect_hdr, strlen(connect_hdr));
      Rio_writen(clientfd, (void*)proxy_connect_hdr, strlen(proxy_connect_hdr));


/*
  readline save it in the buffer,chekc if the thing ends w/ \r\n,
  done reading when length is 0
  readnb
*/


       size_t bufSize = 0;

       Rio_readinitb(&rio, fd);
       size_t lineSize = Rio_readlineb(&rio, serverBuf, MAXLINE);

       while (lineSize > 0) {
         bufSize += lineSize;
         /*if (strcmp(serverBuf[bufSize-4],"\r\n", 4) == 0) {
           Rio_writen(connfd, serverBuf, bufSize);
           break;
           }*/
         lineSize = Rio_readlineb(&rio, serverBuf, MAXLINE);
       }
       Rio_writen(fd, serverBuf, bufSize);


       clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /* Parse URI from GET request */



    if (stat(filename, &sbuf) < 0) {
	clienterror(fd, filename, "404", "Not found",
		    "Tiny couldn't find this file");
	return;
    }

    if (is_static) { /* Serve static content */
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	    return;
	}
	serve_static(fd, filename, sbuf.st_size);
    }
    else { /* Serve dynamic content */
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	    return;
	}
	serve_dynamic(fd, filename, cgiargs);
    }
}
/* $end doit */

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
/*
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin")) {  Static content */
/*	strcpy(cgiargs, "");
	strcpy(filename, ".");
	strcat(filename, uri);
	if (uri[strlen(uri)-1] == '/')
	    strcat(filename, "home.html");
	return 1;
    }
   else {   Dynamic content */
/*	ptr = index(uri, '?');
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	}
	else
	    strcpy(cgiargs, "");
	strcpy(filename, ".");
	strcat(filename, uri);
	return 0;
    }
}
*/
/* $end parse_uri */

int parse_uri(char* buf, char* host, char* uri, char* version,
                                     char* portnm, char* htmlfile)
{
  char url[MAXLINE];
  char hostaddr[MAXLINE];
  char resource[MAXLINE];
  int offset = 7;
  int start;
  int stat = 1;

  if(buf[strlen(buf)-1] != '/') stat = 1;

  sscanf(buf, "%s %s %s", method, url, version);
  //set hostaddr to the url following "http://"
  strcpy(hostaddr, url + offset);

  start = strcspn(hostaddr, "/");
  char* hostname = hostaddr + start;
  int portnum = strcspn(hostname, ":");
  strncpy(uri, hostname, portnum);
  char* portfile = hostname + portnum;
  int spos = strcspn(portfile, "/");
  strncpy(portnm, portfile, spos);
  strcpy(htmlfile, portfile + spos);

  //parse uri
  strcpy(resource, hostname + portnum);
  int htmlfilenum = strcspn(resource, "/");
  strcpy(htmlfile, resource + htmlfilenum);
  strncpy(portnm, resource, htmlfilenum - portnum);
  return stat;
}


    /*
 * serve_static - copy a file back to the client
 */

/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
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
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) { /* child */
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1);
	Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */
	Execve(filename, emptylist, environ); /* Run CGI program */
    }
    Wait(NULL); /* Parent waits for and reaps child */
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
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

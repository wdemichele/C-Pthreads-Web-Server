// Multithreaded HTTP Server in C subsmission for COMP30023 Project 2
// Submission by William De Michele, Student ID: 1071003, 20/05/2022

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <pthread.h>
#include <ctype.h>

#define MAX_BACKLOG 10
#define MAX_RESPONSE 1024000
#define IMPLEMENTS_IPV6
#define MULTITHREADED

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// socket structure to pass (void *) argument in pthread_create for multithreading
struct sock_struct {
    char *path;
    int client_fd[3000];
    int curr;
}arg;

void *http_request(void *arguments);
int validPort(char s[]);
int server_socket(char *port, char *ipv_type);
int response(int fd, char *header, char *content_type, char *body, int body_sz);
void response_404(int fd, char *path);
void response_200(int fd, char *path, char *type);

int main(int argc, char *argv[]) {

    char addr[INET6_ADDRSTRLEN];
    pthread_t p_thread[3000];
    int i=0;
    int client_fd, client_fd6;
    struct sockaddr_storage client_addr;
    struct sockaddr_in6 client_addr6;   
    
    // only accepts ipv4 or ipv6
    if (strcmp(argv[1],"4") != 0 && strcmp(argv[1],"6") != 0) {
        exit(1);
    }

    // checks for validity of port
    if (validPort(argv[2]) == 0){
        exit(1);
    }

    int server_fd = server_socket(argv[2], argv[1]);

    // ensures server socket is successfully bound to an ip address
    if (server_fd < 0) {
        exit(1);
    }
    arg.path = argv[3];
    arg.curr = 0;

    while(1) {
        
        // slight differences between ipv4 and ipv6 struct socket address

        if (strcmp(argv[1],"6") == 0) { // ipv6 client socket assignment
            socklen_t client_size6 = sizeof client_addr6;

            // accepts ipv6 connection
            client_fd6 = accept(server_fd, (struct sockaddr *)&client_addr6, &client_size6);
            if (client_fd6 == -1) {
                continue;
            }
            inet_ntop(AF_INET6, &client_addr6.sin6_port,addr, sizeof addr);
            arg.curr++;
            arg.client_fd[arg.curr] = client_fd6;
            
            
        } 
        else { // ipv4 client socket assignment

            socklen_t client_size = sizeof client_addr;

            // accepts ipv4 connection
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_size);
            if (client_fd == -1) {
                continue;
            }
            arg.curr++;
            arg.client_fd[arg.curr] = client_fd;
        }
        // creates thread to perform http_request function using ipv6 arguments
        pthread_create(&p_thread[i++],NULL,http_request,(void *)&arg);
        // http_request((void *)&arg);
  }
  return 0;
}

void *http_request(void *arguments) {
    // reads the request and determines the response to send based on the request
    
    const int buffer_sz = 2024; // 2kB
    char request[buffer_sz], request_type[8], path[1024], protocol[128];
    
    FILE *file;
    struct sock_struct args = *((struct sock_struct *)arguments);
    int client_fd = args.client_fd[arg.curr];
    // receive message from client socket
    int received = recv(client_fd, request, buffer_sz - 1, 0);

    // checks there was a message sent
    if (received < 0) {
        return NULL;
    }

    // appends string ending
    request[received] = '\0';

    // scans request options from the request
    sscanf(request, "%s %s %s", request_type, path, protocol);

    // creates absolute path from requested file and web root path
    char *fullpath = args.path;
    char absPath[1024];
    strcpy(absPath,fullpath);
    strcat(absPath,path);

    // ensures that path doesn't attempt to access illegal areas
    if ((strstr(absPath, "../")) != NULL || (strstr(absPath, ":")) != NULL || (strstr(absPath, "Makefile")) != NULL || (strstr(absPath, "makefile")) != NULL){
        response_404(client_fd, absPath);
    }

    // ensures request is of type GET
    if (strcmp(request_type, "GET") == 0) {
        // checks file request is valid
        
        if ((file = fopen(absPath, "rb"))) {
            fclose(file);

            // determines file type by grabbing last 3/4/5 characters of file, ensuring the file is of this or greater size
            if (strlen(absPath) >= 3 && absPath[strlen(absPath)-3] == '.' && absPath[strlen(absPath)-2] == 'j' && absPath[strlen(absPath)-1] == 's') {
                response_200(client_fd, absPath,"text/javascript");
            } else if (strlen(absPath) >= 4 && absPath[strlen(absPath)-4] == '.' && absPath[strlen(absPath)-3] == 'c' && absPath[strlen(absPath)-2] == 's' && absPath[strlen(absPath)-1] == 's') {
                response_200(client_fd, absPath,"text/css");
            } else if (strlen(absPath) >= 4 && absPath[strlen(absPath)-4] == '.' && absPath[strlen(absPath)-3] == 'j' && absPath[strlen(absPath)-2] == 'p' && absPath[strlen(absPath)-1] == 'g') {
                response_200(client_fd, absPath,"image/jpeg");
            } else if (strlen(absPath) >= 5 && absPath[strlen(absPath)-5] == '.' && absPath[strlen(absPath)-4] == 'h' && absPath[strlen(absPath)-3] == 't' && absPath[strlen(absPath)-2] == 'm' && absPath[strlen(absPath)-1] == 'l') {
                response_200(client_fd, absPath,"text/html");
            } else {
                //application octet-stream if otherwise unidentified
                response_200(client_fd, absPath,"application/octet-stream");
            }
        } else {
            // file request is invalid
            response_404(client_fd, absPath);
        }
    }
    // can only handle GET requests
    else {
        response_404(client_fd, absPath);
    }
    close(client_fd);
    // thread finished
    // pthread_exit(NULL);
    return(NULL);
}

int validPort(char s[]) {
    // ensures that all characters of the provided port are digits
    for (int i = 0; s[i]!= '\0'; i++)
    {
        if (isdigit(s[i]) == 0)
              return 0;
    }
    return 1;
}

int server_socket(char *port, char *ipv_type) {
    // returns the server listening socket to request

    int sockfd, return_value;
    struct addrinfo addr, *server_addr, *p;

    memset(&addr, 0, sizeof addr);
    addr.ai_family = AF_UNSPEC; // either ipv4 or ipv6
    addr.ai_flags = AI_PASSIVE; // any IP on the machine

    // get non-zero address info
    if ((return_value = getaddrinfo(NULL, port, &addr, &server_addr)) != 0) {
        return -1;
    }

    // loop through until we are able to set up a socket on an interface with port
    for(p = server_addr; p != NULL; p = p->ai_next) {

        // set up ipv6 socket
        if (strcmp(ipv_type,"6") == 0) {
            sockfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if(sockfd == -1) {
                continue;
	        }
        } // set up ipv4 socket
        else {
            if ((sockfd = socket(p->ai_family, SOCK_STREAM, p->ai_protocol)) == -1) {
                continue;
            }
        }

        // prevents attempting to use an address that is already being used
        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            perror("setsockopt");
            exit(1);
        }
        if (strcmp(ipv_type,"6") == 0) {

            // binding using modified address info for ipv6
            struct sockaddr_in6 server_addr;
            int iport = atoi(port);
            server_addr.sin6_family = AF_INET6;
            server_addr.sin6_addr = in6addr_any;
            server_addr.sin6_port = htons(iport);
            
            // bind socket to ipv6 address
            if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
                close(sockfd);
                continue;
            }
        } else {
            // bind socket to ipv4 address
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                continue;
            }
        }
        // if we get to here then the processes of binding a socket to the server was successful
        break;
        }
        freeaddrinfo(server_addr); 

    // no possible interface to bind to was found
    if (p == NULL)  {
        return -1;
    }

    // listen using the socket we just bound to the interface
    if (listen(sockfd, MAX_BACKLOG) == -1) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int response(int fd, char *header, char *content_type, char *body, int body_sz){
    // function to format and send the response to the client
    char response[MAX_RESPONSE];

    // format the response
    int response_length = sprintf(response,
        "%s\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: %s\r\n"
        "\r\n"
        "%s\r\n",
        header,
        body_sz,
        content_type,
        body);

    // send the formatted response to be read by the client
    
    int return_value = send(fd, response, response_length, 0);
    printf("%s\n", response);

    return return_value;
}

void response_404(int fd, char *path) {
    // 404 response that notifies client that requested file is unavailable, typically
    // as it cannot be found, however in our case also if the file is forbidden
    char response_body[1024];
    sprintf(response_body, "404: %s not found", path);
    response(fd, "HTTP/1.1 404 NOT FOUND", "text/html", response_body, strlen(response_body));
}

void response_200(int fd, char *path, char *type){
    // 200 OK response for when requested file is available for viewing
    char * buffer = 0;
    unsigned long length;

    // only one thread accessing file at a time
    pthread_mutex_lock(&mutex);

    // open in binary mode in event of binary files that include terminal characters
    FILE * f = fopen (path, "rb");

    if (f) { 
        // use fseek to determine the length of the file contents
        fseek (f, 0, SEEK_END);
        length = ftell (f);

        //rewind fseek back to start for file reading
        fseek (f, 0, SEEK_SET);
        buffer = malloc (length);

        // copy file contents to buffer
        if (buffer) {
            fread (buffer, 1, length, f);
            length = strlen(buffer);
        }   
        fclose (f); 
    }
    else {
        // file unavailable, return 404 error
        response_404(fd, path);
    }
    // thread finished with file
    pthread_mutex_unlock(&mutex);

    if (buffer) {
        // reads file string to body for response
        response(fd, "HTTP/1.1 200 OK", type, buffer, length);
    }
    else {
        // no file buffer, return 404
        response_404(fd, path);
    }
}





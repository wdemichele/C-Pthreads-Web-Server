# C Pthreads Web Server

## Programming with TCP/IP Sockets [^1]

    1. Create the Socket
    2. Identify the Socket
    3. Server end: wait for incoming connection
    4. Send and receive messages
    5. Close the Socket

### 1. Create a Socket
Socket is created with the parameters:
    - domain/address family
    - type
    - protocol

Domain or Address Family:
    Communication domain in which socket should be created

Type:
    Type of service, selected according to properties required by application

Protocol:
    Protocol to support socket operation
    Useful where some families may have more than one protocol to support a given type of service


### 2. Identify a Socket
Need to assign a transport address to socket (port number IP networking)
This is known as "binding" an address

Defined in socket address structure
```
int bind(int socket, const struct sockaddr *address, socklen_t address_len);
```

Socket is defined in Step 1
```
struct sockaddr_in 
    { 
        __uint8_t         sin_len; 
        sa_family_t       sin_family; // Address family used to set up socket
        in_port_t         sin_port; // Port number
        struct in_addr    sin_addr; // Machine's IP address
        char              sin_zero[8]; 
    };
```

### 3. Server end: wait for incoming connection
Use the *listen* system call to have a socket that is prepared to accept incoming connections.
```
int listen(int socket, int backlog);
```
Can then *accept* the first request in the *backlog* queue, and create a new socket for this connection.
```
int accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);
```

### 4. Send and receive messages
Sockets are now connected between Client and Server.
Can use simple *read* and *write* calls as per standard file usage in C.

### 5. Close the Socket
Once communication is complete, close the socket
```
close(new_socket);
```

## Server Features
### Get Requests
Load pages, images, JavaScript within the server,

### MIME types
A media type (also known as a Multipurpose Internet Mail Extensions or MIME type) indicates the nature and format of a document, file, or assortment of bytes. MIME types are defined and standardized in IETF's RFC 6838 [^2]

Identifies MIME types for:
- html -> *text/html*
- image -> *image/jpeg*
- css -> *text/css*
- JavaScript -> *text/javascript*
- unknown -> *application/octet-stream*

### IPv6 Implementation
The webroot used in testing can be found under www/.
#### IPv4 cURL request
```
curl --http1.0 -v http://127.0.0.1:PORT/PATH
```
#### IPv6 cURL request
```
curl --http1.0 -v http://[::1]:PORT/PATH
```

### Parallel downloads 
Server uses *p-threads* to process incoming requests and sending responses. 
Does not fork multiple processes.
Server can process and respond to at least five HTTP requests concurrently.

[^1]: https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa
[^2]: https://datatracker.ietf.org/doc/html/rfc6838

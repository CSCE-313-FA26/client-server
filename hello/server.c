/*
Usage: ./server <port number>
Use a non-priveleged port number above 1024

One connection, one message each way, then the server exits.
This is the smallest complete TCP server: socket, bind, listen, accept, talk, close.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno; /* the listening socket, the connected socket, the port */
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in server_address, client_address;
    int n;
    int option = 1;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\nUsage: %s <port number>\n", argv[0]);
        exit(1);
    }

    /*
    Create a server socket
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Family version (IPV4), type (TCP)
    if (sockfd < 0)
        error("ERROR opening socket");

    /*
     Let the port be re-used immediately.
     Without this, a server that has just exited leaves the port in TIME_WAIT for
     about a minute and the next run dies with "bind: Address already in use".
    */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
        error("ERROR on setsockopt");

    /*
     - Get the port number
     - Setup the server_address structure for use in the bind call
     - INADDR_ANY accepts a connection on any of this host's addresses
     - The port number must be converted into network byte order
    */
    bzero((char *)&server_address, sizeof(server_address));

    portno = atoi(argv[1]); // Get the port number from the command line

    server_address.sin_family = AF_INET;          // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY;  // any local address
    server_address.sin_port = htons(portno);      // !important: host -> network byte order

    /*
     Bind the socket to the port.
     bind(int fd, struct sockaddr *local_addr, socklen_t addr_length)
    */
    if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        error("ERROR on binding");

    /*
     Listen for incoming connections.
     Connections wait in a backlog queue until accept() takes one. Queue size: 5.
    */
    if (listen(sockfd, 5) < 0)
        error("ERROR on listen");

    printf("Server listening on port %d\n", portno);
    fflush(stdout);

    /*
     accept() blocks until a client connects, then returns a SECOND socket --
     the one this conversation happens on. sockfd stays open for the next client.
    */
    clilen = sizeof(client_address);
    newsockfd = accept(sockfd, (struct sockaddr *)&client_address, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    printf("Server received a connection!\n");
    fflush(stdout);

    /*
     Send a greeting on the connected socket.
     Use strlen() for the length. Counting the bytes by hand is how you lose the
     last character of a string: "Hello from the server!\n" is 23 bytes, not 22.
    */
    const char *greeting = "Hello from the server!\n";
    if (send(newsockfd, greeting, strlen(greeting), 0) < 0)
        error("ERROR writing to socket");

    /*
     Then read what the client has to say.
    */
    bzero(buffer, 256);
    n = read(newsockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("Message from client: %s", buffer);
    fflush(stdout);

    close(newsockfd);
    close(sockfd);
    return 0;
}

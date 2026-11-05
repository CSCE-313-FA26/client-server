/*
Usage: ./client localhost <server port number>

One connection, one message each way, then the client exits.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in server_address;
    struct hostent *server;
    char buffer[256]; // Data to send and receive

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(1);
    }

    /*
    Get the port number
    */
    portno = atoi(argv[2]);

    /*
    Create the socket. A client needs no bind() -- the kernel picks a local port.
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP
    if (sockfd < 0)
        error("ERROR opening socket");

    /*
    Look the server's name up. "localhost" resolves to 127.0.0.1.
    */
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host: %s\n", argv[1]);
        exit(1);
    }

    /*
    server_address is a struct sockaddr_in: an address family, an IP address and a port.
    AF_INET says the address is an IPv4 address.
    */
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(portno);

    /*
    Ask for a connection. connect() blocks until the server's kernel accepts it.
    */
    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        error("ERROR connecting");

    /*
    The server speaks first, so read its greeting before sending anything.
    */
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s", buffer);

    printf("Please enter the message: ");
    fflush(stdout);
    bzero(buffer, 256);
    if (fgets(buffer, 255, stdin) == NULL)
        buffer[0] = '\0';

    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");

    close(sockfd);
    return 0;
}

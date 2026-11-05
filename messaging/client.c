/*
Usage: ./client localhost <server port number>

Sends messages typed at the keyboard to the server and prints each answer.
Type "bye", or press Ctrl+D, to disconnect.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 256

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
    char buffer[BUFFER_SIZE];
    int saying_bye;

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(1);
    }

    portno = atoi(argv[2]);

    /*
    Create the socket. A client needs no bind(): the kernel picks a local port.
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
    if (sockfd < 0)
        error("ERROR opening socket");

    /*
    Look up the server's name. "localhost" resolves to 127.0.0.1.
    */
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host: %s\n", argv[1]);
        exit(1);
    }

    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET; // IPv4
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(portno);

    /*
    Ask for the connection. This is the client's half of the three-way handshake.
    */
    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        error("ERROR connecting");

    printf("Connected to %s on port %d\n", argv[1], portno);

    /*
    The server speaks first, so read its greeting before sending anything.
    */
    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s", buffer);

    /*
    Then take turns: send a line, read the answer.
    */
    for (;;)
    {
        printf("Please enter the message: ");
        fflush(stdout);

        bzero(buffer, BUFFER_SIZE);
        if (fgets(buffer, BUFFER_SIZE - 1, stdin) == NULL)
        {
            /* Ctrl+D. Closing the socket is what tells the server we are done. */
            printf("\n");
            break;
        }

        saying_bye = (strncmp(buffer, "bye", 3) == 0 &&
                      (buffer[3] == '\n' || buffer[3] == '\0'));

        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");

        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n < 0)
            error("ERROR reading from socket");
        if (n == 0)
        {
            printf("Server closed the connection\n");
            break;
        }
        printf("Server: %s", buffer);

        if (saying_bye)
            break; /* the server has said goodbye and closed its end */
    }

    close(sockfd);
    return 0;
}

/*
Usage: ./server <port number>
Use a non-priveleged port number above 1024

A message server. It serves one client at a time:

    accept a connection
      -> exchange messages with that client until it says "bye" or closes
    -> go back and accept the next connection

The two loops are the whole idea. The OUTER loop is one client after another on
the listening socket. The INNER loop is one message after another on the
connected socket.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 256

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/*
 Strip the trailing newline fgets() left on the client's line, so the message
 prints on one line and compares equal to "bye".
*/
void trim_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    char reply[BUFFER_SIZE * 2];
    struct sockaddr_in server_address, client_address;
    char peer[INET_ADDRSTRLEN];
    int n, i, message_count, client_count = 0;
    int option = 1;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\nUsage: %s <port number>\n", argv[0]);
        exit(1);
    }

    /*
    Create the listening socket
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
    if (sockfd < 0)
        error("ERROR opening socket");

    /*
     Let the port be re-used immediately. Without this, a server that has just
     exited leaves the port in TIME_WAIT for about a minute, and the next run
     dies with "bind: Address already in use".
    */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
        error("ERROR on setsockopt");

    bzero((char *)&server_address, sizeof(server_address));
    portno = atoi(argv[1]);
    server_address.sin_family = AF_INET;         // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY; // any local address
    server_address.sin_port = htons(portno);     // host -> network byte order

    if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        error("ERROR on binding");

    if (listen(sockfd, 5) < 0) // backlog queue of 5 pending connections
        error("ERROR on listen");

    printf("Server listening on port %d\n", portno);
    printf("Press Ctrl+C to stop the server.\n");
    fflush(stdout);

    /*
     OUTER LOOP: one client after another.
     accept() blocks here until someone connects. It returns a SECOND socket --
     the conversation happens on newsockfd, while sockfd stays open for the
     next client.
    */
    for (;;)
    {
        clilen = sizeof(client_address);
        newsockfd = accept(sockfd, (struct sockaddr *)&client_address, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        client_count++;
        inet_ntop(AF_INET, &client_address.sin_addr, peer, sizeof(peer));
        printf("\n[client %d] connected from %s:%d\n",
               client_count, peer, ntohs(client_address.sin_port));
        fflush(stdout);

        /*
         The server speaks first. Use strlen() for the length: counting bytes by
         hand is how you lose the last character of a string.
        */
        const char *greeting = "Hello from the server! Send a message, or 'bye' to disconnect.\n";
        if (send(newsockfd, greeting, strlen(greeting), 0) < 0)
        {
            perror("ERROR writing to socket");
            close(newsockfd);
            continue;
        }

        /*
         INNER LOOP: one message after another on this connection.
        */
        message_count = 0;
        for (;;)
        {
            bzero(buffer, BUFFER_SIZE);
            n = read(newsockfd, buffer, BUFFER_SIZE - 1);

            if (n < 0)
            {
                perror("ERROR reading from socket");
                break;
            }
            if (n == 0)
            {
                /* read() returning 0 is end-of-file: the client closed its end. */
                printf("[client %d] closed the connection\n", client_count);
                break;
            }

            trim_newline(buffer);
            message_count++;
            printf("[client %d] message %d (%d bytes): %s\n",
                   client_count, message_count, n, buffer);
            fflush(stdout);

            if (strcmp(buffer, "bye") == 0)
            {
                const char *farewell = "Goodbye!\n";
                send(newsockfd, farewell, strlen(farewell), 0);
                printf("[client %d] said bye\n", client_count);
                break;
            }

            /*
             Answer. Shouting the message back in capitals proves the server
             really processed it rather than the client echoing its own text.
            */
            for (i = 0; buffer[i] != '\0'; i++)
                buffer[i] = toupper((unsigned char)buffer[i]);

            snprintf(reply, sizeof(reply), "message %d received (%d bytes): %s\n",
                     message_count, n, buffer);

            if (send(newsockfd, reply, strlen(reply), 0) < 0)
            {
                perror("ERROR writing to socket");
                break;
            }
        }

        close(newsockfd); /* close the CONVERSATION, not the listening socket */
        printf("[client %d] disconnected after %d message(s). Waiting for the next client...\n",
               client_count, message_count);
        fflush(stdout);
    }

    close(sockfd); /* never reached: stop the server with Ctrl+C */
    return 0;
}

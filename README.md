# Client and server with the socket API

Two working TCP programs for CSCE 313, Introduction to Computer Systems.
Texas A&M University, Department of Computer Science and Engineering.

Author: David Kebo Houngninou

Both programs run on one machine. The client connects to the server over
`localhost`, they exchange messages, and the connection closes. Nothing here
needs a network, a second computer or root.

```
hello/        one connection, one message each way, then both programs exit
messaging/    the server serves one client after another, each for as many
              messages as that client wants to send
```

Build everything:

```bash
make
```

Both directories compile with **zero warnings** under `-Wall -Wextra`.

## hello/ — the smallest complete pair

Seven calls are the whole server: `socket`, `setsockopt`, `bind`, `listen`,
`accept`, `send`/`read`, `close`. Three are the whole client: `socket`,
`connect`, `write`/`read`.

Two terminals. Server first:

```
$ cd hello && make
$ ./server 8080
Server listening on port 8080
Server received a connection!
Message from client: Hello from the client!
```

Then the client:

```
$ ./client localhost 8080
Hello from the server!
Please enter the message: Hello from the client!
```

The server exits after that one conversation, because there is no loop.

## messaging/ — two loops

```
accept a connection                  <-- OUTER loop: one client after another
  exchange messages until "bye"      <-- INNER loop: one message after another
go back and accept the next client
```

The server replies to each message with its length and the message in capitals,
so you can see it really processed the text rather than the client echoing its
own line.

Server terminal, after one client sent three messages and a second sent one and
pressed Ctrl+D:

```
$ ./server 8080
Server listening on port 8080
Press Ctrl+C to stop the server.

[client 1] connected from 127.0.0.1:47298
[client 1] message 1 (12 bytes): hello there
[client 1] message 2 (34 bytes): sockets are just file descriptors
[client 1] message 3 (4 bytes): bye
[client 1] said bye
[client 1] disconnected after 3 message(s). Waiting for the next client...

[client 2] connected from 127.0.0.1:47312
[client 2] message 1 (27 bytes): second client, same server
[client 2] closed the connection
[client 2] disconnected after 1 message(s). Waiting for the next client...
```

Client 1's terminal:

```
$ ./client localhost 8080
Connected to localhost on port 8080
Hello from the server! Send a message, or 'bye' to disconnect.
Please enter the message: hello there
Server: message 1 received (12 bytes): HELLO THERE
Please enter the message: sockets are just file descriptors
Server: message 2 received (34 bytes): SOCKETS ARE JUST FILE DESCRIPTORS
Please enter the message: bye
Server: Goodbye!
```

Type `bye` to disconnect politely, or press **Ctrl+D** to just close the socket.
The server handles both: `bye` is a message it recognises, and a closed socket
makes its `read()` return **0**, which is end-of-file.

Stop the server with **Ctrl+C**.

## Things worth pointing at while this runs

**`accept()` returns a second socket.** `sockfd` is the one that listens;
`newsockfd` is the one the conversation happens on. Closing `newsockfd` ends one
conversation. Closing `sockfd` stops the server. Mixing the two up is the
commonest mistake in this code.

**A client needs no `bind()`.** The kernel assigns it a local port, which is the
number after the colon in `connected from 127.0.0.1:47298`. Run two clients and
that number differs — that is what distinguishes the two connections.

**`read()` returning 0 is not an error.** It is end-of-file: the peer closed its
end. A server that treats 0 as "keep going" spins at 100% CPU; one that treats
it as an error reports a failure that did not happen.

**`htons()` is not decoration.** Network byte order is big-endian. Drop
`htons()` on a little-endian machine and port 8080 becomes 36895, so the client
cannot find the server and `connect` fails with "Connection refused".

**`SO_REUSEADDR` is why you can restart the server.** Without that
`setsockopt()` call, a server that has just exited leaves the port in `TIME_WAIT`
for about a minute and the next run dies with `bind: Address already in use`.
Comment the call out and try it — it is the fastest way to make the point. Watch
the state with:

```bash
ss -tn | grep 8080
```

**One `read()` is not one message.** These programs get away with a single
`read()` because every message is short and typed slowly. TCP is a byte stream:
it is free to split one `send()` across two `read()`s, or deliver two `send()`s
in one `read()`. That is why Lab 6 gives you `send_all` and `recv_all`.

**Watch the handshake.** On another terminal, before connecting:

```bash
sudo tcpdump -i lo -n port 8080
```

You will see SYN, SYN-ACK, ACK when the client connects, and FIN when it closes.

## Ports are per-machine

Pick your own port above 1024. In a shared lab two people on 8080 collide, and
the second one gets `bind: Address already in use` — and `SO_REUSEADDR` does
**not** help there, because the port belongs to someone else's live process, not
to your own `TIME_WAIT`. See what is in use with `ss -ltn`.

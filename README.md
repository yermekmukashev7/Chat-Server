# Chat Server

-TCP Chat Server implemented in C was a project for my Computer Networks class at UMD.
-Functionality: client creation with a chosen nickname, private messages between clients, private chat rooms, list users command, and list rooms command.
-Client: Provided as a part of the project (CMSC417, University of Maryland).

## Client Usage:

1. \connect <IP Address>:<Port> = Instruct the client to connect to a new chat server,
specified by the IP address and port.

2. \disconnect = If connected to a server, disconnect from that server.

3. \join <Room> <Password> = Join the specified chatroom, creating it if it does not
    already exist. The Password is optional, with the default being the empty string. Users may
    only join rooms for which they know the password. Both Room and Password must be less
    than 256 characters in length.

4. \leave = If in a room, this exits the room. Otherwise, it leaves the server.

5. \list users = List all users. If in a room, it lists all users in that room. Otherwise, it lists
    all users connected to the server.

6. \list rooms = List all rooms that currently exist on the server.

7. \msg <User> <Message> = Send a private message to the specified user. User must be
    less than 256 characters in length, and the Message must be less than 65536 characters in
    length.

8. \nick <Name> = Set your nickname to the specified name. Name must be less than 256
    characters in length.

9. \quit = Exits out of the client.

## Server Usage:

1. -p <Number> = The port that the server will listen on. Represented as a base-10 integer.
    Must be specified.

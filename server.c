#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <time.h>

#include "server.h"

int clntsNum = 0;
struct clientInfo *clnts;
struct clientInfo *tail;

struct chatrooms *rooms;
struct chatrooms *roomtail;

char nicks[MAXCLNTS];

/* Buffer for payloads sent by clients */
char recvbuffer[MAXBUFFER];

void createClnt(int socketfd) {
    char nick[10];
    int num, j, k, len;

    for (int i = 0; i < MAXCLNTS; i++) {
        if (nicks[i] != -1) {
            num = nicks[i];
            nicks[i] = -1;
            break;
        }
    }
    memset(nick, 0, 10);
    sprintf(nick, "rand%d", num);
    len = strlen(nick);

    char payload[NICKPROTO + len];

    payload[0] = '\x00';
    payload[1] = '\x00';
    payload[2] = '\x00';
    payload[3] = len + 1;
    payload[4] = '\x04';
    payload[5] = '\x17';
    payload[6] = '\x9a';
    payload[7] = '\x00';

    j = 8;
    k = 0;
    while (nick[k] != 0) {
        payload[j++] = nick[k++];
    }

    if (!clnts) {
        clnts = (struct clientInfo*)malloc(sizeof(struct clientInfo));
        clnts->next = NULL;
        clnts->room = NULL;
        clnts->nickDef = 1;
        clnts->socketfd = socketfd;
        clnts->name = (char *)malloc(len + 1);
        strcpy(clnts->name, nick);
        clnts->name[len] = '\0';
        tail = clnts;
    } else {
        tail->next = (struct clientInfo*)malloc(sizeof(struct clientInfo));
        tail = tail->next;
        tail->next = NULL;
        tail->room = NULL;
        clnts->nickDef = 1;
        tail->socketfd = socketfd;
        tail->name = (char *)malloc(len + 1);
        strcpy(tail->name, nick);
        tail->name[len] = '\0';
    }

    send(socketfd, &payload, NICKPROTO + len, 0);
}

void dltClnt(int socketfd) {

    struct clientInfo *j, *i = clnts;
    if (clnts->socketfd == socketfd) {
        if (!(clnts->next)) {
            clnts = NULL;
            tail = NULL;
        } else {
            clnts = clnts->next;
        }
        free(i->name);
        if (i->room) {
            free(i->room);
        } 
        free(i);
        return;
    }

    while (i != NULL) {
        if (i->next->socketfd == socketfd) {
            j = i->next;
            if (!(i->next->next)) {
                i->next = NULL;
                tail = i;
            } else {
                i->next = i->next->next;
            }
            free(j->name);
            if (j->room) {
                free(j->room);
            }
            free(j);
            break;
        }
        i = i->next;
    }
}

void nickUpdt(struct clientInfo *client, char *nick, int len) {

    int exist = 0;
    int num;
    struct clientInfo *i = clnts;

    /* Checking if the nick already exists */
    while (i != NULL) {
        if (strcmp(i->name, nick) == 0) {
            exist = 1;
            break;
        }
        i = i->next;
    }

    if (exist) {
        char payload[50] = "\x00\x00\x00\x2b\x04\x17\x9a\x01" \
                           "\x54\x68\x69\x73\x20\x6e\x69\x63" \
                           "\x6b\x20\x68\x61\x73\x20\x62\x65" \
                           "\x65\x6e\x20\x6e\x69\x63\x6b\x65" \
                           "\x64\x20\x62\x79\x20\x73\x6f\x6d" \
                           "\x65\x6f\x6e\x65\x20\x65\x6c\x73" \
                           "\x65\x2e";

        send(client->socketfd, &payload, 50, 0);
    } else {
        char payload[8] = "\x00\x00\x00\x01\x04\x17\x9a\x00";

        if (client->nickDef) {
            client->nickDef = 0;
            num = atoi(client->name + 4);
            nicks[num] = num;
        }
    
        printf("User %s changed his nick to ", client->name);
        client->name = realloc(client->name, len + 1);
        if (!(client->name)) {
            printf("Realloc error!\n");
        }
        memset(client->name, 0, sizeof(char) * len + 1);
        strcpy(client->name, nick);
        printf("%s\n", client->name);
        printf("Client name len: %ld\n", strlen(client->name));
        send(client->socketfd, &payload, 8, 0);
    }
}

void gibres(int socketfd) {
    char payload[49] = "\x00\x00\x00\x2a\x04\x17\x9a\x01" \
                       "\x59\x6f\x75\x20\x73\x68\x6f\x75" \
                       "\x74\x20\x69\x6e\x74\x6f\x20\x74" \
                       "\x68\x65\x20\x76\x6f\x69\x64\x20" \
                       "\x61\x6e\x64\x20\x68\x65\x61\x72" \
                       "\x20\x6e\x6f\x74\x68\x69\x6e\x67" \
                       "\x2e";

    send(socketfd, &payload, 49, 0);
}

void listusers(struct clientInfo *client) {
    struct clientInfo *i, *list;
    struct chatrooms *r = rooms;
    int len = 0, j, k;

    if (!(client->room)) {
        i = clnts;
        list = clnts;
    } else {
        while (r != NULL) {
            if (strcmp(client->room, r->roomName) == 0) {
                i = r->participants;
                list = r->participants;
                break;
            }
            r = r->next;
        }
    }

    while (i != NULL) {
        len += strlen(i->name) + 1;
        i = i->next;
    }

    char payload[len + LISTUSERPROTO];
    payload[0] = '\x00';
    payload[1] = '\x00';
    payload[2] = '\x00';
    payload[3] = len + 1;
    payload[4] = '\x04';
    payload[5] = '\x17';
    payload[6] = '\x9a';
    payload[7] = '\x00';

    j = 8;
    i = list;
    while (i != NULL) {
        k = 0;
        payload[j++] = strlen(i->name); 
        while (i->name[k] != 0) {
            payload[j++] = i->name[k++];
        }
        i = i->next;
    }
    send(client->socketfd, &payload, len + LISTUSERPROTO, 0);
}

void listRoomClnts(const struct chatrooms *room) {
    struct clientInfo *i = room->participants;
    printf("Room %s users: ", room->roomName);
    while (i != NULL) {
        printf("%s | ", i->name);
        i = i->next;
    }
    putchar('\n');
}

void roomCreate(char *name, char *psw, const struct clientInfo *client) {
    char payload[8] = "\x00\x00\x00\x01\x04\x17\x9a\x00";
    struct clientInfo *clnt;
    struct chatrooms *r;

    if (!rooms) {
        rooms = (struct chatrooms*)malloc(sizeof(struct chatrooms));
        roomtail = rooms;
        r = rooms;
    } else {
        roomtail->next = (struct chatrooms*)malloc(sizeof(struct chatrooms));
        roomtail = roomtail->next;
        r = roomtail;
    }

    r->next = NULL;
    r->roomName = (char*)malloc(strlen(name) + 1);
    strcpy(r->roomName, name);
    printf("****** Room Name Len: %ld\n", strlen(r->roomName));
    printf("Room %s was just created\n", rooms->roomName);
    r->clntsnum = 1;
    if (!psw) {
        r->psw = NULL;
    } else {
        r->psw = (char*)malloc(strlen(psw) + 1);
        strcpy(r->psw, psw);
        printf("Psw: %s\n", rooms->psw);
    }

    /* Adding the clnt to the room */
    r->participants = (struct clientInfo*)malloc(sizeof(struct clientInfo));
    r->tail = r->participants;
    clnt = r->participants;
    clnt->socketfd = client->socketfd;
    clnt->room = NULL;
    clnt->next = NULL;
    clnt->name = (char*)malloc(strlen(client->name) + 1);
    strcpy(clnt->name, client->name);

    send(client->socketfd, &payload, 8, 0);

    listRoomClnts(r);
}

void joinRoom(char *name, char *psw, struct clientInfo *client) {

    struct chatrooms *i = rooms;
    int exist = 0, correctPsw = 0;
    /* Searching for the room */
    while (i != NULL) {
        if (strcmp(i->roomName, name) == 0) {
            exist = 1;
            break;
        }
        i = i->next;
    }

    if (!exist) {
        printf("Room doesn't exist. Creating...\n");
        /* Saving the room name in client's struct */
        client->room = (char*)malloc(strlen(name) + 1);
        strcpy(client->room, name);
        roomCreate(name, psw, client);
    } else {
        /* Checking the password */
        if (!(i->psw) && !psw) {
            correctPsw = 1;
        } else if (i->psw && psw && strcmp(i->psw, psw) == 0) {
            correctPsw = 1;
        }

        if (correctPsw) {
            char payload[8] = "\x00\x00\x00\x01\x04\x17\x9a\x00";
            printf("Correct Password. Joining...\n");

            /* Saving the room name in client's struct */
            client->room = (char*)malloc(strlen(name) + 1);
            strcpy(client->room, name);

            /* Adding the clnt */
            i->clntsnum++;
            printf("Clients in the room: %d\n", i->clntsnum);
            i->tail->next = (struct clientInfo*)malloc(sizeof(struct clientInfo));
            i->tail = i->tail->next;
            i->tail->socketfd = client->socketfd;
            i->tail->room = NULL;
            i->tail->next = NULL;
            i->tail->name = (char*)malloc(strlen(client->name) + 1);
            strcpy(i->tail->name, client->name);
            printf("Client: %s just joined the room %s\n", i->tail->name, i->roomName);
            send(client->socketfd, &payload, 8, 0);
        } else {
            char payload[61] = "\x00\x00\x00\x36\x04\x17\x9a\x01" \
                               "\x49\x6e\x76\x61\x6c\x69\x64\x20" \
                               "\x70\x61\x73\x73\x77\x6f\x72\x64" \
                               "\x2e\x20\x49\x27\x6d\x20\x61\x66" \
                               "\x72\x61\x69\x64\x20\x49\x20\x63" \
                               "\x61\x6e\x27\x74\x20\x6c\x65\x74" \
                               "\x20\x79\x6f\x75\x20\x64\x6f\x20" \
                               "\x74\x68\x61\x74\x2e";

            send(client->socketfd, &payload, 61, 0);
        }
        listRoomClnts(i);
    }
}

void listRooms(int socketfd) {
    struct chatrooms *i = rooms;
    int size = 8, len = 0, j, k;

    while (i != NULL) {
        len += strlen(i->roomName) + 1;
        i = i->next;
    }

    char payload[size + len];
    payload[0] = '\x00';
    payload[1] = '\x00';
    payload[2] = '\x00';
    payload[3] = len + 1;
    payload[4] = '\x04';
    payload[5] = '\x17';
    payload[6] = '\x9a';
    payload[7] = '\x00';

    j = 8;
    i = rooms;
    while (i != NULL) {
        k = 0;
        payload[j++] = strlen(i->roomName);
        while (i->roomName[k] != 0) {
            payload[j++] = i->roomName[k++];
        }
        i = i->next;
    }

    send(socketfd, &payload, size + len, 0);
}

int leave(struct clientInfo *client) {
    struct clientInfo *i, *j;
    struct chatrooms *r = rooms;

    if (!(client->room)) {
        return 1;
    }

    /* Searching for the client's room */
    while (r != NULL) {
        if (strcmp(r->roomName, client->room) == 0) {
            i = r->participants;
            break;
        }
        r = r->next;
    }

    free(client->room);
    client->room = NULL;

    /* Removing the client */
    if (i->socketfd == client->socketfd) {
        if (!(i->next)) {
            r->participants = NULL;
            r->tail = NULL;
        } else {
            r->participants = i->next;
        }

        free(i->name);
        free(i);
        r->clntsnum--;
        if (r->clntsnum == 0) {
            removeEmptyRoom(r->roomName);
        }
        return 0;
    } else {
        while (i != NULL) {
            if (i->next->socketfd == client->socketfd) {
                if (!(i->next->next)) {
                    r->tail = i;
                    free(i->next->name);
                    free(i->next);
                    i->next = NULL;
                } else {
                    j = i->next;
                    i->next = i->next->next;
                    free(j->name);
                    free(j);
                }

                r->clntsnum--;
                if (r->clntsnum == 0) {
                    removeEmptyRoom(r->roomName);
                }
                return 0;
            }
            i = i->next;
        }
    }
    return -1;
}

void removeEmptyRoom(char *name) {
    struct chatrooms *i = rooms, *j;
    
    if (strcmp(i->roomName, name) == 0) {
        if (!(i->next)) {
            rooms = NULL;
            roomtail = NULL;
        } else {
            rooms = i->next;
        }
        free(i->roomName);
        free(i->psw);
        free(i->participants);
        free(i);
        return;
    }
    
    while (i != NULL) {
        if (strcmp(i->next->roomName, name) == 0) {
            if (!(i->next->next)) {
                roomtail = i;
                free(i->next->roomName);
                free(i->next->psw);
                free(i->next->participants);
                free(i->next);
                i->next = NULL;
            } else {
                j = i->next;
                i->next = i->next->next;
                free(j->roomName);
                free(j->psw);
                free(j->participants);
                free(j);
            }
            return;        
        }
        i = i->next;
    }
}

void sendMsg(int size, char *room, char *msg, struct clientInfo *client) {
    char payload[8] = "\x00\x00\x00\x01\x04\x17\x9a\x00";
    int len = size + strlen(client->name) + 1;
    int j, k;
    char message[len + JOINPROTO];
    struct clientInfo *i;
    struct chatrooms *r = rooms;

    while (r != NULL) {
        if (strcmp(r->roomName, room) == 0) {
            i = r->participants;
            break;
        }
        r = r->next;
    }

    send(client->socketfd, &payload, 8, 0);

    message[0] = '\x00';
    message[1] = '\x00';
    message[2] = '\x00';
    message[3] = len;
    message[4] = '\x04';
    message[5] = '\x17';
    message[6] = '\x15';
    message[7] = strlen(room);

    j = 8;
    k = 0;
    while (room[k] != 0) {
        message[j++] = room[k++];
    }

    message[j++] = strlen(client->name);
    k = 0;
    while (client->name[k] != 0) {
        message[j++] = client->name[k++];
    }
    message[j++] = '\x00';
    message[j++] = strlen(msg);
    k = 0;
    while (msg[k] != 0) {
        message[j++] = msg[k++];
    }


    while (i != NULL) {
        if (strcmp(i->name, client->name) != 0) {
            send(i->socketfd, &message, len + JOINPROTO, 0);
        }
        i = i->next;
    }
}

void prvtMsg(char *user, char *msg, struct clientInfo *client) {
    int len = strlen(client->name) + strlen(msg) + 3;
    struct clientInfo *i = clnts;
    int j, k, present = 0;
    char message[len + JOINPROTO];

    while (i != NULL) {
        if (strcmp(i->name, user) == 0) {
            present = 1;
            break;
        }
        i = i->next;
    }

    if (!present) {
        char payload[24] = "\x00\x00\x00\x11\x04\x17\x9a\x01" \
                           "\x4e\x69\x63\x6b\x20\x6e\x6f\x74" \
                           "\x20\x70\x72\x65\x73\x65\x6e\x74";


        send(client->socketfd, &payload, 24, 0);

    } else {
        char payload[8] = "\x00\x00\x00\x01\x04\x17\x9a\x00";
        send(client->socketfd, &payload, 8, 0);

        message[0] = '\x00';
        message[1] = '\x00';
        message[2] = '\x00';
        message[3] = len;
        message[4] = '\x04';
        message[5] = '\x17';
        message[6] = '\x12';
        message[7] = strlen(client->name);

        j = 8;
        k = 0;
        while (client->name[k] != 0) {
            message[j++] = client->name[k++];
        }
        message[j++] = '\x00';
        message[j++] = strlen(msg);
        k = 0;
        while (msg[k] != 0) {
            message[j++] = msg[k++];
        }

        send(i->socketfd, &message, len + JOINPROTO, 0);
    }
}


void printClnts() {
    if (!(clnts)) {
        printf("List is empty\n");
        return;
    }
    struct clientInfo *i = clnts;
    while (i != NULL) {
        printf("Name: %s\n", i->name);
        i = i->next;
    }
}

double diff(const struct timespec *time1, const struct timespec *time0) {
    struct timespec diffTime = {.tv_sec = time1->tv_sec - time0->tv_sec, .tv_nsec = time1->tv_nsec - time0->tv_nsec};
    if (diffTime.tv_nsec < 0) {
        diffTime.tv_nsec += 1000000000;
        diffTime.tv_sec--;
    }
    return diffTime.tv_sec + diffTime.tv_nsec / TOSECONDS;
}

int main(int argc, char *argv[]) {

    struct clientInfo *client;
    struct sockaddr_in servAddr;
    struct pollfd fds[MAXCLNTS];
    int pr;
    int socketClient;
    int timeout, nfds = 1, sNum = 0;
    int closeServ = 0, closeClnt, compress = 0;
    int bytesToRead;
    int size, roomlen, pswlen, gb, msgLen, senderlen, num;
    int port = atoi(argv[2]);

    if (argc != 3) {
        printf("Not enough arguments\n");
    }
    

    int socketd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(port); 
    bind(socketd, (struct sockaddr *)&servAddr, sizeof(servAddr));
    listen(socketd, MAXCLNTS);

    memset(fds, 0 , sizeof(fds));

    fds[0].fd = socketd;
    fds[0].events = POLLIN;
    timeout = (WAITTIMEMIN * 60 * 1000);

    for (int i = 0; i < MAXCLNTS; i++) {
        nicks[i] = i;
    }

    do {
        pr = poll(fds, nfds, timeout);

        if (pr == 0) {
            printf("!!!!!!!No staying alive were received!!!!!!!\n");
            struct clientInfo *i = clnts;
            int socket;

            if (clnts) {
                compress = 1;
                for (int j = 1; j < nfds; j++) {
                    fds[j].fd = -1;
                }
                while (i != NULL) {
                    if (i->nickDef) {
                        num = atoi(i->name + 4);
                        nicks[num] = num;
                    }
                    socket = i->socketfd;
                    if (i->room) {
                        leave(i);
                    }
                    close(i->socketfd);
                    dltClnt(socket);
                }
            }
            timeout = (WAITTIMEMIN * 60 * 1000);
        } else {

            sNum = nfds;
            for (int i = 0; i < sNum; i++) {

                /* Find the client */
                client = clnts;
                while (client != NULL) {
                    if (client->socketfd == fds[i].fd) {
                        break;
                    }
                client = client->next;
                }

                if (fds[i].revents == 0) {
                    printf("Here\n");
                    continue;
                }

                if(fds[i].revents != POLLIN) {
                    printf("Error!\n");
                    closeServ = 1;
                    break;         
                }

                if (fds[i].fd == socketd) {
                    printf("New client(s) is ready\n");
                    // do {
                    socketClient = accept(socketd, NULL, NULL);

                    /* Receiving Bonghjornu message from the incoming client */
                    recv(socketClient, recvbuffer, 68, 0);
                    
                    /* Adding the new incoming client */
                    createClnt(socketClient);
                    timeout = ALIVEINTERVAL * 1000;
                    fds[nfds].fd = socketClient;
                    fds[nfds].events = POLLIN;
                    nfds++;
                    printClnts();
                    // } while (socketClient != -1);


                } else {
                    closeClnt = 0;
                    /* Clearing the buffer before recv() */
                    memset(recvbuffer, 0, MAXBUFFER);
                    pr = recv(fds[i].fd, recvbuffer, MAXBUFFER, MSG_PEEK);

                    /* Client closed connection */
                    if (pr == 0) {
                        closeClnt = 1;
                        if (client->room) {
                            leave(client);
                        }
                    }
                    
                    else {
                        gb = 0;

                        /* Staying Alive Msg */
                        if (pr == ALIVELEN && recvbuffer[6] == '\x13' && recvbuffer[7] == '\x1f') {
                            char alive[ALIVELEN];
                            pr = recv(fds[i].fd, alive, ALIVELEN, 0);
                            // printf("Received: %d, Alive MSG\n", pr); 
                        }

                        /* Gibberish Msg */
                        if (recvbuffer[6] == '\x15' && recvbuffer[3] - 3 == recvbuffer[9]) {
                            size = recvbuffer[9] + GIBPROTO;
                            char tempbuff[size];
                            pr = recv(fds[i].fd, tempbuff, size, 0);
                            gibres(fds[i].fd);
                            gb = 1;
                        }

                        /*********** Proccess Different Commands ***********/
                        /* \nick <Name> */
                        if (recvbuffer[6] == '\x0f' && recvbuffer[7] + 1 == recvbuffer[3]) {
                            printf("$$$$Nick Request$$$$\n");
                            size = recvbuffer[7];
                            bytesToRead = size + NICKPROTO;
                            char tempbuff[bytesToRead];
                            char nick[size + 1];
                            pr = recv(fds[i].fd, tempbuff, bytesToRead, 0);
                            strncpy(nick, tempbuff + 8, size);
                            nick[size] = '\0';
                            nickUpdt(client, nick, size);
                            printClnts();
                            putchar('\n');
                        }

                        /* \list users */
                        if (recvbuffer[3] == '\x00' && recvbuffer[6] == '\x0c') {
                            printf("$$$$List Users Request$$$$\n");
                            char tempbuff[LISTUSERLEN];
                            pr = recv(fds[i].fd, tempbuff, LISTUSERLEN, 0);
                            printf("Received Bytes LU: %d\n", pr);
                            listusers(client);
                            putchar('\n');
                        }

                        /* \join <Room> <Password> */
                        if (recvbuffer[6] == '\x03') {
                            printf("$$$$Join Room Request$$$$\n");
                            size = recvbuffer[3];
                            roomlen = recvbuffer[7];
                            pswlen = recvbuffer[JOINPROTO + roomlen + 1];
                            char tempbuff[size + JOINPROTO];
                            
                            char *psw;
                            char roomname[roomlen + 1];
                            strncpy(roomname, recvbuffer + 8, roomlen);
                            roomname[roomlen] = '\0';

                            if (pswlen == 0) {
                                psw = NULL;
                            } else {
                                psw = (char*)malloc(pswlen + 1);
                                memset(psw, 0, pswlen + 1);
                                strncpy(psw, recvbuffer + 8 + roomlen + 1, pswlen);
                            }

                            pr = recv(fds[i].fd, tempbuff, size + JOINPROTO, 0);

                            if (client->room) {
                                leave(client);
                            }
                            joinRoom(roomname, psw, client);
                            putchar('\n');
                        }

                        /* \list rooms */
                        if (recvbuffer[3] == '\x00' && recvbuffer[6] == '\x09') {
                            printf("$$$$List Rooms Request$$$$\n");
                            char tempbuff[LISTUSERLEN];
                            pr = recv(fds[i].fd, tempbuff, LISTUSERLEN, 0);
                            printf("Received Bytes LR: %d\n", pr);
                            listRooms(fds[i].fd);
                            putchar('\n');
                        }

                        /* \leave */
                        if (recvbuffer[3] == '\x00' && recvbuffer[6] == '\x06') {
                            printf("$$$$Leave Request$$$$\n");
                            char payload[8] = "\x00\x00\x00\x01\x04\x17\x9a\x00";
                            char tempbuff[LEAVEPROTO];
                            pr = recv(fds[i].fd, tempbuff, LEAVEPROTO, 0);
                            pr = leave(client);
                            if (pr == 1) {
                                closeClnt = 1;
                                send(fds[i].fd, &payload, 8, 0);
                            } else if (pr == -1) {
                                printf("Leave Error!\n");
                                closeServ = 1;
                            } else {
                                printf("The Clnt was removed from the room\n");
                                send(fds[i].fd, &payload, 8, 0);
                            }
                            putchar('\n');
                        }

                        /* Group Chat */
                        if (recvbuffer[6] == '\x15' && !gb) {
                            printf("$$$$Group Chat Message Request$$$$\n");
                            size = recvbuffer[3];
                            char tempbuff[size + JOINPROTO];
                            roomlen = recvbuffer[7];
                            printf("Room Len: %d\n", roomlen);
                            char roomname[roomlen + 1];
                            msgLen = recvbuffer[JOINPROTO + roomlen + 2];
                            printf("Msg Len: %d\n", msgLen);
                            char msg[msgLen + 1];
                            strncpy(roomname, recvbuffer + 8, roomlen);
                            strncpy(msg, recvbuffer + (JOINPROTO + roomlen + 3), msgLen);
                            roomname[roomlen] = '\0';
                            msg[msgLen] = '\0';
                            pr = recv(fds[i].fd, tempbuff, size + JOINPROTO, 0);
                            printf("Bytes Received: %d\n", pr);
                            printf("Room: %s, Msg: %s\n", roomname, msg);
                            sendMsg(size, roomname, msg, client);
                        }

                        /* Private Msg */
                        if (recvbuffer[6] == '\x12') {
                            printf("$$$$Private Message Request$$$$\n");
                            size = recvbuffer[3];
                            char tempbuff[size + JOINPROTO];
                            senderlen = recvbuffer[7];
                            char sender[senderlen + 1];
                            strncpy(sender, recvbuffer + 8, senderlen);
                            sender[senderlen] = '\0';
                            msgLen = recvbuffer[JOINPROTO + senderlen + 2];
                            char msg[msgLen + 1];
                            strncpy(msg, recvbuffer + (JOINPROTO + senderlen + 3), msgLen);
                            msg[msgLen] = '\0';
                            pr = recv(fds[i].fd, tempbuff, size + JOINPROTO, 0);
                            printf("Sender: %s\n", sender);
                            printf("Msg: %s\n", msg);
                            prvtMsg(sender, msg, client);
                        }
                    }

                    if (closeClnt == 1) {
                        if (client->nickDef) {
                            num = atoi(client->name + 4);
                            nicks[num] = num;
                        }
                        dltClnt(fds[i].fd);
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        compress = 1;
                    }
                }
            }
        }

        if (compress) {
            compress = 0;
            for (int k = 0; k < nfds; k++) {
                if (fds[k].fd == -1) {
                    for(int j = k; j < nfds; j++) {
                        fds[j].fd = fds[j+1].fd;
                    }
                    k--;
                    nfds--;
                }
            }
        }

        printf("Here1\n");
    } while(closeServ == 0);

    
    return EXIT_SUCCESS;
}
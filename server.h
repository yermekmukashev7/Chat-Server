#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define WAITTIMEMIN 10
#define MAXCLNTS 256
#define MAXBUFFER 120
#define ALIVEINTERVAL 6.3
#define TOSECONDS 1000000000.0

#define LEAVEPROTO 7
#define JOINPROTO 7
#define NICKPROTO 8
#define LISTUSERPROTO 8
#define GIBPROTO 10


#define ALIVELEN 39
#define LISTUSERLEN 7

struct clientInfo {   
    struct clientInfo *next;

    char *name;
    char *room;
    int nickDef;
    int socketfd;
} clientInfo;

struct chatrooms {
    struct chatrooms *next;

    int clntsnum;
    char *roomName, *psw;
    struct clientInfo *participants, *tail;

} chatrooms;

void createClnt(int socketfd);

void dltClnt(int socketfd);

void nickUpdt(struct clientInfo *client, char *nick, int len);

void gibres(int socketfd);

void listusers(struct clientInfo *client);

void roomCreate(char *name, char *psw, const struct clientInfo *client);

void joinRoom(char *name, char *psw, struct clientInfo *client);

void listRooms(int socketfd);

int leave(struct clientInfo *client);

void removeEmptyRoom(char *name);

void sendMsg(int size, char *room, char *msg, struct clientInfo *client);

void prvtMsg(char *user, char *msg, struct clientInfo *client);
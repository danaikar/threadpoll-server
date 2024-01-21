#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
# include <sys/wait.h>
# include <netdb.h> 
# include <ctype.h> 
#include <string.h>
#include <signal.h>

#define MAX_PARTIES 100
#define MAX_NUM 100

typedef struct {
    int portnum;
    int numWorkerThreads;
    int bufferSize;
    char pollLog[MAX_NUM];
    char pollStats[MAX_NUM];
} Info;

typedef struct {
    char party[MAX_NUM];
    int votes;
} PartyStats;

PartyStats *pollStats;   // array of PartyStats structs
char* pollStatsFile;     // file name
int totalParties;

int *buffer;
int bufferIndex;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;
int num_clients;



typedef struct {
    char serverName[MAX_NUM];
    int portNum;
    char inputFile[MAX_NUM];
} ClientInfo;

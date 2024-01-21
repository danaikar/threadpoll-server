#include "headers.h"


void handleSIGPIPE(int signal) { }

void handleSIGINT(int signal) {
    
    FILE* file = fopen(pollStatsFile, "w");
    if (file == NULL) {
        printf("Failed to open poll-stats\n");
        exit(1);
    }
    
    fprintf(file, "total parties: %d\n", totalParties);
    fprintf(file, "names: \n");
    for (int i = 0; i <= totalParties; i++)
        fprintf(file, "%s\n", pollStats[i].party);

    for (int i = 0; i <= totalParties; i++) {
        if (strlen(pollStats[i].party) > 0) {
            fprintf(file, "%s %d\n", pollStats[i].party, pollStats[i].votes);
        }
    }
    
    int totalVotes = 0;
    for (int i = 0; i <= totalParties; i++) {
        totalVotes += pollStats[i].votes;
    }

    fprintf(file, "TOTAL %d\n", totalVotes);
    fclose(file);
    exit(0);
}


int hasVoted(char *name, char *surname) {
    
    FILE* votedFile = fopen("voted.txt", "a");
    if (votedFile == NULL) {
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), votedFile) != NULL) {
        // remove newline character at the end of the line
        size_t length = strlen(line);
        if (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[length - 1] = '\0';
        }
        if (strcmp(line, name) == 0 && strcmp(line, surname) == 0) {  // has already voted
            fprintf(votedFile, "VOTEDDDDDDDDDDDDDDD!");
            fclose(votedFile);
            return 1;
        }
        fprintf(votedFile, "%s = %s %s\n", line, name, surname);
    }

    fprintf(votedFile, "%s %s\n", name, surname);
    fflush(votedFile);  
    fclose(votedFile);
    return 0;
}


void* workerThread(void* arg) {

    Info* info = (Info*)arg;
    int client;
    char nameBuffer[MAX_NUM];
    char surnameBuffer[MAX_NUM];
    char partyBuffer[MAX_NUM];
    char generalBuffer[3*MAX_NUM];

    while (1) {

        pthread_mutex_lock(&mutex);  // lock the mutex to ensure that only one thread can access/modify the array 

        /* if the buffer is full, wait on the bufferNotEmpty condition variable.
           so put the thread to sleep, until it is woken up by a signal */
        while (bufferIndex == 0) {   
            pthread_cond_wait(&bufferNotEmpty, &mutex);
        }

        /* remove the connection from the buffer and signal the bufferNotEmpty condition
           variable to let other threads know there is now space available in the buffer */
        client = buffer[--bufferIndex];
        pthread_cond_signal(&bufferNotFull);
        pthread_mutex_unlock(&mutex);

        /* recieve name and vote from client */
        memset(generalBuffer, 0, sizeof(generalBuffer));
        recv(client, generalBuffer, sizeof(generalBuffer) - 1, 0); 

        sscanf(generalBuffer, "%s %s %s", nameBuffer, surnameBuffer, partyBuffer);  // split these 3

        printf("NAME: %s SURNAME: %s PARTY: %s\n", nameBuffer, surnameBuffer, partyBuffer);

        /* check if client has already voted */
        if (hasVoted(nameBuffer, surnameBuffer)) {
            printf("!!!!!!!! ALREADY VOTED !!!!!!!!!\n");
            shutdown(client, SHUT_RDWR);  // if yes, ignore this client and loop to the next connection
            continue;
        }


        /* update pollStats */
        for (int i = 0; i < MAX_PARTIES; i++) {
            if (pollStats[i].party[0] == '\0') {
                strcpy(pollStats[i].party, partyBuffer);
                pollStats[i].votes = 1;
                totalParties++;
                break;
            } else if (strcmp(pollStats[i].party, partyBuffer) == 0) {
                pollStats[i].votes++;
                break;
            }
        }

        /* open file and write */
        FILE* logFile = fopen(info->pollLog, "a");
        if (logFile != NULL) {
            printf("VOTE for Party %s RECORDED\n", partyBuffer);
            fprintf(logFile, "%s %s %s\n", nameBuffer, surnameBuffer, partyBuffer);
            fflush(logFile); 
            fclose(logFile);
        }
                
        /* close connection */
        close(client);
    }

    return NULL;
}




void* masterThread(void* arg) {
    
    Info* info = (Info*)arg;
    int portnum = info->portnum;
    int maxClients = info->bufferSize;
    int bufferSize = info->bufferSize;

    int sock, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    struct sockaddr *clientptr = (struct sockaddr *)&clientAddr;
    struct sockaddr *serverptr = (struct sockaddr *)&serverAddr;
    socklen_t clientlen;

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror(" socket ");
        exit(1);
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portnum);

    /* Bind socket to address */
    if (bind(sock, serverptr, sizeof(serverAddr)) < 0) {
        perror(" bind ");
        exit(1);
    }

    /* Listen for connections */
    if(listen(sock , maxClients) < 0) {
        perror(" listen ");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", portnum);

    while (1) {
        /* accept connection */
        clientlen = sizeof(clientAddr);

        if ((clientSocket = accept(sock, clientptr, &clientlen)) < 0) {  // blocks until a client connection request is received
            perror(" accept ");
            exit(EXIT_FAILURE);
        }
                        
        pthread_mutex_lock(&mutex);  // lock the mutex to ensure that only one thread can access/modify the array */

        /* if the buffer is full, wait on the bufferNotFull condition variable.
           so put the thread to sleep, until it is woken up by a signal */
        while(bufferIndex == bufferSize) {   
            pthread_cond_wait(&bufferNotFull, &mutex);
        }

        /* add the connection to the buffer and signal the bufferNotEmpty condition variable to 
           let other threads know that there is at least one connection in the buffer waiting */
        buffer[bufferIndex++] = clientSocket;
        pthread_cond_signal(&bufferNotEmpty);
        pthread_mutex_unlock(&mutex);  // unlock, so other threads can access the buffer
    }

    return NULL;
}



int main(int argc, char* argv[]) {

    if (argc != 6) {
        printf("Wrong number of arguments\n");
        return -1;
    }
    
    signal(SIGINT, handleSIGINT);
    signal(SIGPIPE, handleSIGPIPE);

    // fill the info struct with the arguments
    Info info;
    info.portnum = atoi(argv[1]);
    info.numWorkerThreads = atoi(argv[2]);
    info.bufferSize = atoi(argv[3]);
    strcpy(info.pollLog, argv[4]);
    strcpy(info.pollStats, argv[5]);

    buffer = malloc(info.bufferSize*sizeof(int));
    bufferIndex = 0;

    pollStats = malloc(MAX_PARTIES*sizeof(pollStats));
    pollStatsFile = info.pollStats;
    
    totalParties = 0;

    pthread_t master;
    pthread_t workers[info.numWorkerThreads];

    // create start and wait the master thread and the working threads
    pthread_create(&master, NULL, masterThread, &info);
    for (int i = 0; i < info.numWorkerThreads; i++) {
        pthread_create(&workers[i], NULL, workerThread, &info);
    }
    pthread_join(master, NULL);
    for (int i = 0; i < info.numWorkerThreads; i++) {
        pthread_join(workers[i], NULL);
        if (pthread_join(*(workers + i), NULL) != 0) {
            fprintf(stderr, "Failed to join thread %d\n", i);
            exit(1);
        }
    }
    
    free(buffer);
    free(pollStats);
    return 0;
}
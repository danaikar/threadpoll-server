#include "headers.h"


void* sendVote(void* arg) {
    /* get the client-arguments struct */
    ClientInfo* clientInfo = (ClientInfo*)arg;
    
    /* open the input file */ 
    FILE* inputFile = fopen(clientInfo->inputFile, "r");
    if (inputFile == NULL) {
        perror("Failed to open input file");
        fclose(inputFile);
        pthread_exit(NULL);
    }
    
    char name[MAX_NUM];
    char surname[MAX_NUM];
    char vote[MAX_NUM];
    
    /* read lines from the input file and process each vote */
    while(fscanf(inputFile, "%s %s %s", name, surname, vote) != EOF) {
        /* Create socket */
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            perror("Failed to create socket");
            break;
        }

        /* find server address */
        struct hostent *rem;
        if ((rem = gethostbyname(clientInfo->serverName)) == NULL ) {
            herror (" gethostbyname "); 
            exit (1);
        }
        struct sockaddr_in server;
        server.sin_family = AF_INET; 
        memcpy(& server.sin_addr, rem->h_addr, rem->h_length);
        server.sin_port = htons( clientInfo->portNum); 
        
        /* initiate connection */
        if(connect(clientSocket, (struct sockaddr*)&server, sizeof(server)) < 0) {
            perror(" Connect ");
            exit(1);
        }
        
        /* send the vote to the server */
        char message[3*MAX_NUM];
        sprintf(message, "%s %s %s", name, surname, vote);
        if(send(clientSocket, message, strlen(message), 0) < 0) {
            perror(" Failed to send vote to server ");
            close(clientSocket);
            break;
        }
        printf("vote sent for NAME: %s, SURNAME: %s, PARTY: %s\n", name, surname, vote);
            
        /* close the client socket */
        close(clientSocket);
    }
    
    /* close the file and exit */
    fclose(inputFile);
    pthread_exit(NULL);    
}



int main(int argc, char* argv[]) {

    if (argc != 4) {
        printf("Wrong number of arguments\n");
        return -1;
    }

    // fill the client-info struct with the arguments
    ClientInfo clientInfo;
    strncpy(clientInfo.serverName, argv[1], MAX_NUM);
    clientInfo.portNum = atoi(argv[2]);
    strncpy(clientInfo.inputFile, argv[3], MAX_NUM);

    // create start and wait the vote thread 
    pthread_t threadId;
    if (pthread_create(&threadId, NULL, sendVote, (void*)&clientInfo) != 0) {
        perror("Failed to create vote thread");
        return 1;
    }
    pthread_join(threadId, NULL);

    return 0;
}
    
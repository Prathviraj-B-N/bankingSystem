#include <stdio.h>
#include <fcntl.h>      
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>    
#include <string.h>  
#include <stdbool.h> 
#include <netinet/ip.h> 
#include <stdlib.h>  
#include <errno.h>

#define ACCOUNT_FILE "./records/account.bank"
#define CUSTOMER_FILE "./records/customer.bank"

#include "./functions.h"

void main()
{
    int sockfd, sb, connfd;
    struct sockaddr_in serverAddress, clientAddress;

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // sock_stream and dgram
    if (sockfd == -1)
    {
        perror("creating server socket!");
        _exit(0);
    }

    serverAddress.sin_family = AF_INET;                
    serverAddress.sin_port = htons(3001);              
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); 

    sb = bind(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (sb == -1)
    {
        perror("binding!");
        _exit(0);
    }

    printf("Server started ... \n");

    if (listen(sockfd, 10) == -1) // 10 clients can be in wait queue
    {
        perror("listening for connections!");
        close(sockfd);
        _exit(0);
    }

    int clientSize;
    while (1)
    {
        clientSize = (int)sizeof(clientAddress);
        connfd = accept(sockfd, (struct sockaddr *)&clientAddress, &clientSize);
        if (connfd == -1)
        {
            perror("connecting to client!");
            close(sockfd);
        }
        else
        {
            if (!fork())
            {
                // Child will enter this branch
                printf("Client has connected to the server!\n");

                char readBuffer[1000], writeBuffer[1000];
                ssize_t readBytes, writeBytes;
                int userChoice;

                writeBytes = write(connfd, "Welcome,\n1. Admin\t2. Customer!", strlen("Welcome,\n1. Admin\t2. Customer!"));
                if (writeBytes == -1)
                    perror("sending first prompt to the user!");
                else
                {
                    bzero(readBuffer, sizeof(readBuffer));
                    readBytes = read(connfd, readBuffer, sizeof(readBuffer));
                    if(readBytes == -1)
                        perror("reading from client");
                    else if (readBytes == 0)
                        printf("No data was sent by the client");
                    else
                    {
                        userChoice = atoi(readBuffer);
                        switch (userChoice)
                        {
                        case 1:
                            admin(connfd);
                            break;
                        case 2:
                            customer(connfd);
                            break;
                        default:
                            writeBytes = write(connfd, "closing$", strlen("closing $"));
                            close(connfd);
                            break;
                        }
                    }
                }
                printf("Terminating connection to client!\n");
                close(connfd);
                _exit(0);
            }
        }
    }   
    close(sockfd);
}
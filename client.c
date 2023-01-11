#include <stdio.h>      
#include <sys/socket.h> 
#include <sys/types.h>  
#include <unistd.h>     
#include <fcntl.h>   
#include <string.h>     
#include <netinet/ip.h> 
#include <errno.h>     





void main()
{
    int socketfd;
    struct sockaddr_in serverAddress;
    struct sockaddr server;

    socketfd = socket(AF_INET, SOCK_STREAM, 0); // full duplex byte stream
    if (socketfd == -1)
    {
        perror("creating server socket!");
        _exit(0);
    }

    serverAddress.sin_port = htons(3001); // convert to big Endian         
    serverAddress.sin_family = AF_INET;                
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // automatically assign ip

    if (connect(socketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("connecting to server!");
        close(socketfd);
        _exit(0);
    }

    char readBuffer[1000], writeBuffer[1000]; 
    ssize_t readBytes, writeBytes;

    char tempBuffer[1000];

    do
    {
        bzero(readBuffer, sizeof(readBuffer)); 
        bzero(tempBuffer, sizeof(tempBuffer));
        readBytes = read(socketfd, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
            perror("reading from client socket!");
        else if (strchr(readBuffer, '^') != NULL)
        {
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 1);
            printf("%s\n", tempBuffer);
            writeBytes = write(socketfd, "^", strlen("^"));
            if (writeBytes == -1)
            {
                perror("writing to client socket!");
                break;
            }
        }
        else if (strchr(readBuffer, '$') != NULL)
        {
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 2);
            printf("%s\n", tempBuffer);
            printf("Closing connection...\n");
            break;
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer)); 
            printf("%s\n", readBuffer);
            scanf("%[^\n]%*c", writeBuffer); 

            writeBytes = write(socketfd, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("writing to client!");
                break;
            }
        }
    } while (readBytes > 0);

    close(socketfd);
}



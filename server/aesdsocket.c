#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#define SOCK_DATA_FILE "/var/tmp/aesdsocketdata"
#define LENGTH_OF_BUFFER 512

bool acceptedExit = false;

void printUsage(void)
{
    printf("set the default port and ip .\r\n");
    
}

bool file_create(FILE **file)
{

    

    *file = fopen(SOCK_DATA_FILE, "w");

    if (!*file)
    {
        perror("Creating a new file failed.");
        syslog(LOG_ERR, "Creating a new file failed.");
        return false;
    }

    printf("creating file succeeded.\r\n");
    syslog(LOG_INFO, "creating file succeeded.");
    return true;
}

bool file_remove(FILE **file)
{
    //before removing file has to be closed
    if (access(SOCK_DATA_FILE, F_OK) != 0){
        perror("ERROR::CLOSING FILE:");
        syslog(LOG_INFO, "there is not file: %s.",SOCK_DATA_FILE);
        return false;
    }
    
    int err = fclose(*file);
    if (err)
    {
        perror("ERROR::CLOSING FILE:");
        syslog(LOG_INFO, "closing file failed.");
        return false;
    }
    syslog(LOG_INFO, "closing file succeeded.");

    err = remove(SOCK_DATA_FILE);
    if (err)
    {
        perror("ERROR::REMOVING FILE:");
        syslog(LOG_ERR, "removing file failed.\r\n");
        return false;
    }

    syslog(LOG_INFO, "removing file succeeded.");

    return true;
}

bool server_createSocket(int *sockfd)
{
    if (!sockfd)
        return false;

    *sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*sockfd < 0)
    {
        perror("Creating a new socket failed.");
        syslog(LOG_ERR, "Creating a new socket failed.");
        return false;
    }
    return true;
}

bool server_connectSocket(int *sockfd, struct sockaddr_in *serverAddr)
{
    int err = bind(*sockfd, (struct sockaddr *)serverAddr, sizeof(struct sockaddr));
    if (err < 0)
    {
        syslog(LOG_ERR, "Binding to IP and port failed.");
        return false;
    }

    err = listen(*sockfd, 10);
    if (err)
    {
        syslog(LOG_ERR, "Binding to IP and port failed.");
        return false;
    }
    syslog(LOG_DEBUG, "Accepted connection from xxx");
    return true;
}

bool server_receiveData(FILE **file, int *sockfd)
{

    int buffer[LENGTH_OF_BUFFER];

    if (recv(*sockfd, buffer, LENGTH_OF_BUFFER, 0) < 0)
        return false;

    size_t retSize = fwrite(buffer, sizeof(int), LENGTH_OF_BUFFER, *file);
    if (retSize < 0)
        return false;

    return true;
}

void server_exit(FILE **file, int *sockfd)
{

    // if (!file_remove(file))
    //     exit(EXIT_FAILURE);

    int err = close(*sockfd);

    if (err)
    {
        perror("ERROR::CLOSING SOCKET: ");
        syslog(LOG_ERR, "closing socket falied.\r\n");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_ERR, "closing socket succeeded.");
    syslog(LOG_DEBUG, "Caught signal, exiting.");

    closelog();

    exit(EXIT_SUCCESS);
}

static void signal_acceptExit(int signalType)
{
    int saved_errno = errno;

    if (signalType == SIGINT || signalType == SIGTERM)
        acceptedExit = true;
    printf("acceptedExit is now true\r\n");
    printf("type of signal is %d\r\n", signalType);
    errno = saved_errno;
}

void signal_create()
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction)); // clear buffer
    action.sa_handler = signal_acceptExit;        // set signal handler

    int err = sigaction(SIGTERM, &action, NULL);
    if (err)
    {
        perror("SIGNAL::CREATE SIGTERM:");
        syslog(LOG_ERR, "creating SIGTERM failed.");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_ERR, "creating SIGTERM succeeded.");

    err = sigaction(SIGINT, &action, NULL);
    if (err)
    {
        perror("SIGNAL::CREATE SIGINT:");
        syslog(LOG_ERR, "creating SIGINT failed.");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_ERR, "creating SIGINT succeeded.");
}

int main(int argc, char *argv[])
{

    int err = 1;
    const char *ip = NULL;
    in_port_t port = 9000;
    openlog(NULL, 0, LOG_USER); // open system logger

    // check  arguments
    if (argc != 3)
    {
        printUsage();
    }
    else
    {
        ip = argv[1];
        port = atoi(argv[2]);
    }

    signal_create();

    // create a stream socket
    int sockfd;
    if (!server_createSocket(&sockfd))
        return err;

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr)); // clear buffer of struct

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = port;

    if (!server_connectSocket(&sockfd, &serverAddr))
        return err;
    printf("Accepted connection from %d.\r\n", serverAddr.sin_addr.s_addr);

    FILE *file ;
    if (!file_create(&file))
        return err;

    
    
    while (1)
    {
        // if (!server_receiveData(file, &sockfd))
        // {
        //     syslog(LOG_ERR, "recieving data failed\r\n");
        //     return err;
        // }
        if (acceptedExit){
            file_remove(&file);
            exit(0);
            // server_exit(&file, &sockfd);
        }
            // break;
    }

    return 0;
}
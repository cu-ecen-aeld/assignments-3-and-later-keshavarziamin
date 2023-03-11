#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#define SOCK_DATA_FILE "/var/tmp/aesdsocketdata"
#define LENGTH_OF_BUFFER 512
#define RETERR -1

#define __EXIT(msg_)                    \
    {                                   \
        perror("ERROR:" msg_ ">>");     \
        syslog(LOG_ERR, "ERROR:" msg_); \
        exit(RETERR);                   \
    }

bool acceptedExit = false;
int enable = 1;



void signal_create(void);
void server_createSocket(int *serverfd);
void file_create(FILE *file);
void file_remove(FILE *file);
void server_connectSocket(int *serverfd, struct sockaddr_in *serverAddr, const uint16_t port);
void server_waitToAcceptCient(int *serverfd, struct sockaddr_in *clientAddr);

void signal_acceptExit(int signalType);
void server_receiveData(FILE *file, int *serverfd);
void server_exit(FILE *file, int *serverfd);
void printUsage(void);

int main(int argc, char *argv[])
{
    struct sockaddr_in serverAddr, clientAddr;
    in_port_t port = 9000;
    openlog(NULL, 0, LOG_USER); // open system logger
    uint8_t rxbuff[LENGTH_OF_BUFFER + 1] = {0};

    signal_create();

    // create a stream socket
    int serverfd;
    server_createSocket(&serverfd);
    server_connectSocket(&serverfd, &serverAddr, port);
    server_waitToAcceptCient(&serverfd, &clientAddr);

    // FILE *file;
    // file_create(file);
    int num_read = 0;
    while (1)
    {
        num_read = read(clientAddr,)
    }

    return 0;
}

void file_create(FILE *file)
{
    file = fopen(SOCK_DATA_FILE, "a+");
    if (!file)
        __EXIT("creating file failed.")
}

void server_createSocket(int *serverfd)
{
    if (!serverfd)
    {
        printf("pointer of serverfd is null.\r\n");
        exit(RETERR);
    }

    *serverfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (*serverfd == -1)
        __EXIT("creating socket failed.")

    int err = setsockopt(*serverfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (err)
        __EXIT("Setting option on socket failed.")
}

void signal_create(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(struct sigaction)); // clear buffer
    action.sa_handler = signal_acceptExit;        // set signal handler

    int err = sigaction(SIGTERM, &action, NULL);
    if (err)
        __EXIT("creating signal(SIGTERM) fialed.")

    err = sigaction(SIGINT, &action, NULL);
    if (err)
        __EXIT("creating signal(SIGINT) failed.")
}

void file_remove(FILE *file)
{
    // before removing file has to be closed
    if (access(SOCK_DATA_FILE, F_OK) != 0)
        __EXIT("accessing to file failed.");

    int err = fclose(file);
    if (err)
        __EXIT("closing file failed.");

    err = remove(SOCK_DATA_FILE);
    if (err)
        __EXIT("removing file failed.");
}

void server_connectSocket(int *serverfd, struct sockaddr_in *serverAddr, const uint16_t port)
{

    memset(serverAddr, 0, sizeof(struct sockaddr_in)); // clear buffer of struct
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr.s_addr = INADDR_ANY;
    serverAddr->sin_port = port;

    int err = bind(*serverfd, (struct sockaddr *)serverAddr, sizeof(struct sockaddr));
    if (err < 0)
        __EXIT("binding socket failed.");

    err = listen(*serverfd, 10);
    if (err)
        __EXIT("listening socket failed.");
}

void server_waitToAcceptCient(int *serverfd, struct sockaddr_in *clientAddr)
{
    char clientIp[INET_ADDRSTRLEN] = {0};
    socklen_t socklen = sizeof(struct sockaddr);
    int clientfd = 0;

    printf("server is watting to connect to client >> \r\n");

    while (1)
    {
        clientfd = accept(*serverfd, (struct sockaddr *)clientAddr, &socklen);
        if (clientfd)
            __EXIT("accepting socket failed.");
    }

    inet_ntop(AF_INET, &clientAddr->sin_addr, clientIp, INET_ADDRSTRLEN);
    printf("Accepted connection from %s\r\n", clientIp);
    syslog(LOG_DEBUG, "Accepted connection from %s", clientIp);
}

void server_receiveData(FILE *file, int *serverfd)
{

    int buffer[LENGTH_OF_BUFFER];

    if (recv(*serverfd, buffer, LENGTH_OF_BUFFER, 0) < 0)
        __EXIT("receiving from client failed.");

    size_t retSize = fwrite(buffer, sizeof(int), LENGTH_OF_BUFFER, file);
    if (retSize < 0)
        __EXIT("writing in file failed.");
}

void signal_acceptExit(int signalType)
{
    int saved_errno = errno;

    if (signalType == SIGINT || signalType == SIGTERM)
        acceptedExit = true;
    printf("acceptedExit is now true\r\n");
    printf("type of signal is %d\r\n", signalType);

    errno = saved_errno;
}

void server_exit(FILE *file, int *serverfd)
{

    file_remove(file);

    close(*serverfd);

    syslog(LOG_DEBUG, "Caught signal, exiting.");
    closelog();

    exit(EXIT_SUCCESS);
}

void printUsage(void)
{
    printf("set the default port and ip.\r\n");
}
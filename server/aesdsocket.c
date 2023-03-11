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

struct net_t
{
    struct sockaddr_in addr;
    int fd;
};

void signal_create(void);
void server_createSocket(struct net_t *server);
void file_create(FILE *file);
void file_remove(FILE *file);
void server_connectSocket(struct net_t *server, const uint16_t port);
void server_waitToAcceptCient(struct net_t *server, struct net_t *client);

void signal_acceptExit(int signalType);
void server_receiveData(FILE *file, int *serverfd);
void server_exit(FILE *file, struct net_t *sever);
void printUsage(void);

int main(int argc, char *argv[])
{
    struct net_t server, client;
    in_port_t port = 9000;
    openlog(NULL, 0, LOG_USER); // open system logger
    // uint8_t rxbuff[LENGTH_OF_BUFFER + 1] = {0};

    signal_create();

    // create a server
    server_createSocket(&server);
    server_connectSocket(&server, port);
    server_waitToAcceptCient(&server,&client);

    // FILE *file;
    // // file_create(file);
    // int num_read = 0;
    // while (1)
    // {
    //     num_read = read(client->addr, )
    // }

    return 0;
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

void server_createSocket(struct net_t *server)
{
    if (!server)
        __EXIT("The pinter of server is null.");

    server->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server->fd == -1)
        __EXIT("creating socket failed.")

    int err = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (err)
        __EXIT("Setting option on socket failed.")
}

void server_connectSocket(struct net_t *server, const uint16_t port)
{

    memset(&server->addr, 0, sizeof(struct sockaddr_in)); // clear buffer of struct
    server->addr.sin_family = AF_INET;
    server->addr.sin_addr.s_addr = INADDR_ANY;
    server->addr.sin_port = port;

    int err = bind(server->fd, (struct sockaddr *)&server->addr, sizeof(struct sockaddr));
    if (err < 0)
        __EXIT("binding socket failed.");

    err = listen(server->fd, 10);
    if (err)
        __EXIT("listening socket failed.");
}

void server_waitToAcceptCient(struct net_t *server, struct net_t *client)
{
    char clientIp[INET_ADDRSTRLEN] = {0};
    socklen_t socklen = sizeof(struct sockaddr);
    client->fd = 0;

    printf("server is watting to connect to client >> \r\n");

    while (1)
    {
        client->fd = accept(server->fd, (struct sockaddr *)&client->addr, &socklen);
        if (client->fd == -1)
            __EXIT("accepting socket failed.");
    }

    inet_ntop(AF_INET, &client->addr.sin_addr, clientIp, INET_ADDRSTRLEN);
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

void server_exit(FILE *file, struct net_t *server)
{

    file_remove(file);

    close(server->fd);

    syslog(LOG_DEBUG, "Caught signal, exiting.");
    closelog();

    exit(EXIT_SUCCESS);
}

void file_create(FILE *file)
{
    file = fopen(SOCK_DATA_FILE, "a+");
    if (!file)
        __EXIT("creating file failed.")
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

void printUsage(void)
{
    printf("set the default port and ip.\r\n");
}
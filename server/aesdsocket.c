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
#define LENGTH_OF_BUFFER 2024
#define RETERR -1

#define __EXIT(msg_)                    \
    {                                   \
        perror("ERROR:" msg_ ">>");     \
        syslog(LOG_ERR, "ERROR:" msg_); \
        unlink(SOCK_DATA_FILE);         \
        exit(RETERR);                   \
    }

#ifdef DEBUG_ACTIVE
#define __SUCCESS() printf("%s succeeded.\r\n", __func__)
#else
#define __SUCCESS()
#endif

bool acceptedExit = false;
int enable = 1;

struct net_t
{
    struct sockaddr_in addr;
    int fd;
};

void signal_create(void);
void daemon_create(char *argv);
void server_createSocket(struct net_t *server);
void file_create(FILE **file);
void file_remove(FILE **file);
void server_connectSocket(struct net_t *server, const uint16_t port);
void server_waitToAcceptClient(struct net_t *server, struct net_t *client);

void signal_acceptExit(int signalType);
void server_echoData(FILE **file, int cleinrfd, uint8_t *rxbuff);
void server_exit(FILE **file, struct net_t *server, struct net_t *client);
void printUsage(void);

int main(int argc, char *argv[])
{
    struct net_t server, client;
    in_port_t port = 9000;
    openlog(NULL, 0, LOG_USER); // open system logger
    uint8_t rxbuff[LENGTH_OF_BUFFER + 1] = {0};

    // create and open file in order to write recived data
    FILE *file = {0};
    file_create(&file);

    // create a connect to socket (localhost)
    server_createSocket(&server);
    server_connectSocket(&server, port);

    // create a daemon if argv == -d
    if (argc == 2)
        daemon_create(argv[1]);

    // create signal
    signal_create();

    while (1)
    {
        // before exiting, the server, client, and file have to be closed.
        if (acceptedExit == true)
            server_exit(&file, &server, &client);

        // accept client before receiving data
        server_waitToAcceptClient(&server, &client);
        if (client.fd == -1)
            continue;

        /*
         * receive and echo data from and to client.
         * data must to be write in file after receiving,
         * then read them before transmmiting.
         */
        server_echoData(&file, client.fd, rxbuff);
    }

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

    __SUCCESS();
}

void daemon_create(char *argv)
{
    if (argv == NULL)
        return;

    pid_t pid = 0;
    if (strcmp(argv, "-d") != 0)
        __EXIT("Creating daemon faield.");

    pid = daemon(0, 0);
    if (pid == -1)
        __EXIT("forking the process failed");
}

void server_createSocket(struct net_t *server)
{
    if (!server)
        __EXIT("The pinter of server is null.");

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->fd == -1)
        __EXIT("creating socket failed.")

    int err = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (err)
        __EXIT("Setting option on socket failed.");

    __SUCCESS();
}

void server_connectSocket(struct net_t *server, const uint16_t port)
{

    memset(&server->addr, 0, sizeof(struct sockaddr_in)); // clear buffer of struct
    server->addr.sin_family = AF_INET;
    server->addr.sin_addr.s_addr = INADDR_ANY;
    server->addr.sin_port = htons(port);

    int err = bind(server->fd, (struct sockaddr *)&server->addr, sizeof(struct sockaddr));
    if (err < 0)
        __EXIT("binding socket failed.");

    err = listen(server->fd, 10);
    if (err)
        __EXIT("listening socket failed.");

    __SUCCESS();
}

void server_waitToAcceptClient(struct net_t *server, struct net_t *client)
{
    char clientIp[INET_ADDRSTRLEN] = {0};
    socklen_t socklen = sizeof(struct sockaddr);

    client->fd = accept(server->fd, (struct sockaddr *)&client->addr, &socklen);

    if (client->fd == 0)
    {
        inet_ntop(AF_INET, &client->addr.sin_addr, clientIp, INET_ADDRSTRLEN);
        syslog(LOG_DEBUG, "Accepted connection from %s", clientIp);
    }

    __SUCCESS();
}

void server_echoData(FILE **file, int clientfd, uint8_t *rxbuff)
{
    int len = 0;
    uint8_t txBuff[LENGTH_OF_BUFFER + 1] = {0};
    while (1)
    {
        // read data from buffer of client
        len = read(clientfd, rxbuff, LENGTH_OF_BUFFER);
        if (len < 0)
            __EXIT("Reading data from the client failed.");
        if (len == 0)
            break;

        // write data on file
        rxbuff[len] = '\0';
        len = fprintf(*file, "%s", (char *)rxbuff);
        if (len == -1)
            __EXIT("writing data on file failed.");

        // return the start of STREAM if there is
        if (rxbuff[len - 1] == '\n')
            rewind(*file);
        // read data written from file
        while (1)
        {
            len = fread(txBuff, 1, LENGTH_OF_BUFFER, *file);
            if (len == 0)
                break;
            if (len == -1)
                __EXIT("Reading from file failed.");

            len = write(clientfd, txBuff, len);
            if (len == -1)
                __EXIT("Echoing data to client failed."); /* code */
        }
    }
    __SUCCESS();
}

void signal_acceptExit(int signalType)
{
    int saved_errno = errno;

    if (signalType == SIGINT || signalType == SIGTERM)
        acceptedExit = true;
    syslog(LOG_DEBUG, "Caught signal, exiting");

    errno = saved_errno;
}

void server_exit(FILE **file, struct net_t *server, struct net_t *client)
{

    if (close(server->fd) == -1)
        __EXIT("closing server faield.");
    // if (close(client->fd) == -1)
    //     __EXIT("closing client failed.");
    char clientIp[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &client->addr.sin_addr, clientIp, INET_ADDRSTRLEN);
    syslog(LOG_DEBUG, "Closed connection from %s", clientIp);

    close(client->fd);

    closelog();

    memset(server, 0, sizeof(struct net_t));
    memset(client, 0, sizeof(struct net_t));

    file_remove(file);

    __SUCCESS();

    exit(EXIT_SUCCESS);
}

void file_create(FILE **file)
{
    *file = fopen(SOCK_DATA_FILE, "a+");
    if (!*file)
        __EXIT("creating file failed.")
}

void file_remove(FILE **file)
{

    int err = fclose(*file);
    if (err)
        __EXIT("closing file failed.");

    unlink(SOCK_DATA_FILE);
}

void printUsage(void)
{
    printf("set the default port and ip.\r\n");
}
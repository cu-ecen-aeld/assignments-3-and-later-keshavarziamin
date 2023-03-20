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
#include <pthread.h>

#define SOCK_DATA_FILE "/var/tmp/aesdsocketdata"
#define LENGTH_OF_BUFFER 2024
#define ERROR -1
#define MAX_ECHO_THREAD 200
#define __EXIT(msg_)                    \
    {                                   \
        perror("ERROR:" msg_ ">>");     \
        syslog(LOG_ERR, "ERROR:" msg_); \
        unlink(SOCK_DATA_FILE);         \
        exit(__LINE__);                 \
    }

#ifdef DEBUG_ACTIVE
#define __SUCCESS() printf("%s succeeded.\r\n", __func__)
#else
#define __SUCCESS()
#endif

struct _arg_echoData
{
    FILE *file;
    int cleinrfd;
    uint8_t *rxbuff;
    pthread_mutex_t mtx;
};

struct net_t
{
    struct sockaddr_in addr;
    int fd;
};

void signal_create(void);
void daemon_create(char *argv);
void server_createSocket(void);
void file_remove(void);
void server_connectSocket(void);
void server_waitToAcceptClient(void);
void signal_acceptExit(int signalType);
void server_echoData(int *cfd);
void server_exit(void);
void printUsage(void);

void server_runMainThread(void);
void thread_createEchoThread(int *clientfd);
static void *server_handleMainThread(void *arg);
static void *server_handleEchoDataThread(void *arg);

struct net_t server, client;
in_port_t port = 9000;
uint8_t rxbuff[LENGTH_OF_BUFFER + 1] = {0};
FILE *file = {0};
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
bool acceptedExit = false;
int enable = 1;
pthread_t thr_main;
pthread_t thr_echolist[MAX_ECHO_THREAD];
void *res_main;
void *res_echo[MAX_ECHO_THREAD];
uint32_t idxecho = 0;

int main(int argc, char *argv[])
{

    // create mutex
    openlog(NULL, 0, LOG_USER); // open system logger

    // create and open file in order to write recived data
    file = fopen(SOCK_DATA_FILE, "a+");
    if (!file)
        __EXIT("creating file failed.")
    printf("creating file succeeded.\n");

    // create a daemon if argv == -d
    if (argc == 2)
    {
        daemon_create(argv[1]);
        printf("creating daemon succeeded.\n");
    }

    // create signal
    signal_create();
    printf("creating signal succeeded.\n");

    // create a connect to socket (localhost)
    server_createSocket();
    server_connectSocket();
    printf("connecting socket succeeded.\n");
    // create the main thread to create clients threads
    server_handleMainThread(NULL);

    // server_runMainThread();
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
    if (pid == ERROR)
        __EXIT("forking the process failed");
}

void server_createSocket(void)
{

    server.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server.fd == ERROR)
        __EXIT("creating socket failed.")

    int err = setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (err)
        __EXIT("Setting option on socket failed.");

    __SUCCESS();
}

void server_connectSocket(void)
{

    memset(&server.addr, 0, sizeof(struct sockaddr_in)); // clear buffer of struct
    server.addr.sin_family = AF_INET;
    server.addr.sin_addr.s_addr = INADDR_ANY;
    server.addr.sin_port = htons(port);

    int err = bind(server.fd, (struct sockaddr *)&server.addr, sizeof(struct sockaddr));
    if (err == ERROR)
        __EXIT("binding socket failed.");

    err = listen(server.fd, 10);
    if (err)
        __EXIT("listening socket failed.");

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

void server_exit(void)
{

    if (close(server.fd) == ERROR)
        __EXIT("closing server faield.");
    // if (close(client.fd) == ERROR)
    //     __EXIT("closing client failed.");
    char clientIp[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &client.addr.sin_addr, clientIp, INET_ADDRSTRLEN);
    syslog(LOG_DEBUG, "Closed connection from %s", clientIp);

    close(client.fd);

    closelog();

    memset(&server, 0, sizeof(struct net_t));
    memset(&client, 0, sizeof(struct net_t));

    file_remove();

    __SUCCESS();

    exit(EXIT_SUCCESS);
}

void file_remove(void)
{

    int err = fclose(file);
    if (err)
        __EXIT("closing file failed.");

    unlink(SOCK_DATA_FILE);
}

void printUsage(void)
{
    printf("set the default port and ip.\r\n");
}

void server_runMainThread(void)
{

    int err = pthread_create(&thr_main, NULL,
                             server_handleMainThread, NULL);
    if (err)
        __EXIT("creating main thread faield.");

    err = pthread_join(thr_main, &res_main);
    if (err)
        __EXIT("joining main thread falied.");

    printf("The main thread has just created.\n");
}

static void *server_handleMainThread(void *arg)
{
    // int err = 0;
    while (1)
    {
        // before exiting, the server, client, and file have to be closed.
        if (acceptedExit == true)
            server_exit();

        // accept client before receiving data
        printf("server is wating to accept client connection.\n");
        server_waitToAcceptClient();
        if (client.fd == ERROR)
            continue;
        /*
         * create thread for each request to connect from client
         * in order to manage multi-client requests.
         *
         * receive and echo data from and to client.
         * data must to be write in file after receiving,
         * then read them before transmmiting.
         */
        thread_createEchoThread(&client.fd);
        printf("Server accepted new client connection .\n");
    }
    return (void *)0;
}

void thread_createEchoThread(int *clientfd)
{
    int err = pthread_create(&thr_echolist[idxecho], NULL,
                             server_handleEchoDataThread, (void *)clientfd);
    if (err)
        __EXIT("creating echo data thread failed.");

    err = pthread_join(thr_echolist[idxecho], &res_echo[idxecho]);
    if (err)
        __EXIT("joining thread failed.");

    printf("a echoData thread [%d] has juct created.\n", idxecho);

    idxecho++;
}
static void *server_handleEchoDataThread(void *clientfd)
{
    if (pthread_mutex_lock(&mtx) != 0) // lock other threads to start process
        __EXIT("locking threads failed.");

    server_echoData((int*)clientfd);

    if (pthread_mutex_unlock(&mtx) != 0)
        __EXIT("unlocking threads failed.");

    return (void *)0;
}
void server_waitToAcceptClient(void)
{
    char clientIp[INET_ADDRSTRLEN] = {0};
    socklen_t socklen = sizeof(struct sockaddr);

    client.fd = accept(server.fd, (struct sockaddr *)&client.addr, &socklen);

    if (client.fd == 0)
    {
        inet_ntop(AF_INET, &client.addr.sin_addr, clientIp, INET_ADDRSTRLEN);
        syslog(LOG_DEBUG, "Accepted connection from %s", clientIp);
    }

    __SUCCESS();
}
void server_echoData(int *cfd)
{
    int len = 0;
    uint8_t txBuff[LENGTH_OF_BUFFER + 1] = {0};
    while (1)
    {
        // before exiting, the server, client, and file have to be closed.
        if (acceptedExit == true)
            server_exit();

        // read data from buffer of client
        len = read(*cfd, rxbuff, LENGTH_OF_BUFFER);
        if (len < 0)
            __EXIT("Reading data from the client failed.");
        if (len == 0)
            break;

        // write data on file
        rxbuff[len] = '\0';
        len = fprintf(file, "%s", (char *)rxbuff);
        if (len == ERROR)
            __EXIT("writing data on file failed.");

        // return the start of STREAM if there is
        if (rxbuff[len - 1] == '\n')
            rewind(file);
        // read data written from file
        while (1)
        {
            len = fread(txBuff, 1, LENGTH_OF_BUFFER, file);
            if (len == 0)
                break;
            if (len == ERROR)
                __EXIT("Reading from file failed.");

            len = write(*cfd, txBuff, len);
            if (len == ERROR)
                __EXIT("Echoing data to client failed."); /* code */
        }
    }
    __SUCCESS();
}
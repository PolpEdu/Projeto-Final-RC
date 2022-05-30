
#ifndef FOO_H
#define FOO_H

#define MAX_COMMAND_LENGTH 50
#define BUFLEN 512
#define BUF_SIZE 1024
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXBOLSAS 6
#define GROUP1 "239.0.0.1"
#define GROUP2 "239.0.0.2"
#define GROUP3 "239.0.0.3"
#define GROUP4 "239.0.0.4"
#define GROUP5 "239.0.0.5"
#define GROUP6 "239.0.0.6"

struct RootUser
{
    char *name;
    char *password;
};
struct NormalUser
{
    char *name;
    char *password;
    int saldo;
    char *bolsa1;
    char *bolsa2;
    struct AcaoList *acoes;
};
struct Acao
{
    char *mercado;
    char *nomestock;
    double currentprice;
};
struct UsrList
{
    struct NormalUser *user;
    struct UsrList *next;
};
struct AcaoList
{
    struct Acao *acao;
    struct AcaoList *next;
};

struct threadinfo
{
    int fd;
    struct NormalUser *user;
};

struct SharedMemory
{
    struct UsrList *users_list;
    struct AcaoList *acao_list;
    struct RootUser *root;
    int refresh_time;
    sem_t sem_write;
};

struct SharedMemory *shm;
struct sockaddr_in si_minha;
struct sockaddr_in si_outra;
struct sockaddr_in addr;
struct sockaddr_in client_addr;
struct hostent *hostPtr;
struct ip_mreq mreq;

void *feed_thread(void *arg);
int udp_server(int PORT, struct AcaoList *acao_list, struct UsrList *users_list, struct RootUser *root);
void tcp_server(int PORT_ADMIN, struct AcaoList *acao_list, struct UsrList *users_list, struct RootUser *root);
void erro(char *msg);
void delete_user(struct UsrList *users_list, char *username);
void list_users(struct UsrList *users_list, int socket_fd, struct sockaddr_in *cli_addr, socklen_t slen );
void list_stocks(struct AcaoList *acao_list);
void refresh_time(char *segundos, int socket_fd, struct sockaddr_in *cli_addr, socklen_t slen );
int user_exists(char *username, struct UsrList *users_list);
void append_user(struct UsrList *users_list, struct NormalUser *user);
void append_acao(struct AcaoList *acao_list, struct Acao *acao);
int userSize(struct UsrList *users_list);
struct NormalUser *get_user(struct UsrList *users_list, int index);
void save_to_file();
struct NormalUser *UserbyName(char *username, struct UsrList *users_list);
void menu();
#endif // FOO_H
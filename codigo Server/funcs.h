
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

#define MAXBOLSAS 6

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

void *pricesVolutality();
void *feed_thread(void *arg);
int check_valid_admin_cred(struct RootUser *root_user, char *name, char *password);
int udp_server(int PORT, struct AcaoList *acao_list, struct UsrList *users_list, struct RootUser *root);
void tcp_server(int PORT_ADMIN, struct AcaoList *acao_list, struct UsrList *users_list, struct RootUser *root);
void erro(char *msg);
void delete_user(struct UsrList *users_list, char *username);
void list_users(struct UsrList *users_list);
void list_stocks(struct AcaoList *acao_list);
void refresh_time(char *segundos);
int user_exists(char *username, struct UsrList *users_list);
void append_user(struct UsrList *users_list, struct NormalUser *user);
void append_acao(struct AcaoList *acao_list, struct Acao *acao);
int get_users_size(struct UsrList *users_list);
int get_acao_size(struct AcaoList *acao_list);
struct NormalUser *get_user(struct UsrList *users_list, int index);
struct Acao *get_acao(struct AcaoList *acao_list, int index);
int get_markets_size(struct AcaoList *acao_list);
void save_to_file();
void write_users_tofile(struct UsrList *users_list);
struct NormalUser *get_user_by_name(char *username, struct UsrList *users_list);
#endif // FOO_H
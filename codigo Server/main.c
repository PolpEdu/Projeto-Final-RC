#include "funcs.h"
int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        printf("Usage: ./stock_server <PORT_CLIENT> <PORT_ADMIN> <config_file>\n");
        exit(-1);
    }

    int PORT = atoi(argv[1]);
    int PORT_ADMIN = atoi(argv[2]);
    char *config_file = argv[3];

    FILE *fp = fopen(config_file, "r");
    
    if (fp == NULL)
    {
        printf("Error opening file.\n");
        exit(-1);
    }
    int initial_users_number = 0;
    struct RootUser *root;
    root = malloc(sizeof(struct RootUser));

    struct UsrList *users_list;
    users_list = malloc(sizeof(struct UsrList));
    users_list->next = NULL;
    users_list->user = NULL;

    struct AcaoList *acao_list;
    acao_list = malloc(sizeof(struct AcaoList));
    acao_list->next = NULL;
    acao_list->acao = NULL;

    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), fp))
    {
       
        if (line[0] == '\n')
        {
            continue;
        }
        if (i == 0)
        {
            char *name = strtok(line, "/");
            char *pass = strtok(NULL, "");
            root->name = malloc(strlen(name) + 1);
            strcpy(root->name, name);
            root->password = malloc(strlen(pass) + 1);
            strcpy(root->password, pass);
            pass[strlen(pass) - 1] = '\0';
        }
        else if (i == 1)
        {
            initial_users_number = atoi(line);
            if (initial_users_number > 10)
            {
                printf("Error: The number of users is greater than 10\n");
                return -1;
            }
        }
        else if (i >= 2 && i <= initial_users_number + 1)
        {
            int len = userSize(users_list);
            if (len >= 10)
            {
                printf("Error: The number of users is greater than 10\n");
                return -1;
            }
            char *name = strtok(line, ";");
            char *pass = strtok(NULL, ";");
            char *saldo = strtok(NULL, ";");
            char *bolsas = strtok(NULL, ";");
            char *bolsas2 = strtok(NULL, ";");
            struct NormalUser *user = malloc(sizeof(struct NormalUser));
            user->name = malloc(strlen(name) + 1);
            strcpy(user->name, name);

            user->password = malloc(strlen(pass) + 1);
            strcpy(user->password, pass);
            user->saldo = atoi(saldo);

            if (bolsas == NULL)
            {
                user->bolsa1 = NULL;
            }
            else
            {
                bolsas[strlen(bolsas) - 1] = '\0';
                user->bolsa1 = malloc(strlen(bolsas) + 1);
                strcpy(user->bolsa1, bolsas);
            }

            if (bolsas2 == NULL)
            {
                user->bolsa2 = NULL;
            }
            else
            {
                bolsas2[strlen(bolsas2) - 1] = '\0';
                user->bolsa2 = malloc(strlen(bolsas2) + 1);
                strcpy(user->bolsa2, bolsas2);
            }
            append_user(users_list, user);
        }
        else
        {
            char *market = strtok(line, ";");
            char *nomeacao = strtok(NULL, ";");
            char *preco = strtok(NULL, ";");

            struct Acao *acao = malloc(sizeof(struct Acao));
            acao->mercado = malloc(strlen(market) + 1);
            strcpy(acao->mercado, market);
            acao->nomestock = malloc(strlen(nomeacao) + 1);
            strcpy(acao->nomestock, nomeacao);

            acao->currentprice = atof(preco);
            append_acao(acao_list, acao);
            i++;
        }
        i++;
    }
    int shmid = shmget(IPC_PRIVATE, sizeof(struct SharedMemory), IPC_CREAT | 0700);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }
    shm = (struct SharedMemory *)shmat(shmid, NULL, 0);

    sem_init(&shm->sem_write, 1, 1);

    shm->users_list = users_list;
    shm->acao_list = acao_list;
    shm->root = root;
    shm->refresh_time = 3;

    if (fork() == 0)
    {
        tcp_server(PORT, acao_list, users_list, root);
    }
    else if (fork() == 0)
    {
        udp_server(PORT_ADMIN, acao_list, users_list, root);
    }
    while (1)
    {
        wait(NULL);
    }
    return 0;
}
#include "funcs.h"

/*
    Adicionar um utilizador com uma identificação e uma password, bem como especificar a que
    mercados pode ter acesso (no máximo existem 2 mercados diferentes) e o saldo da sua
    conta; caso já exista um utilizador com aquele nome e password apenas alterará as bolsas a
    que tem acesso e/ou o saldo. O número de utilizadores está limitado a 10 (além do
    administrador).
    ▪ ADD_USER {username} {password} {bolsas a que tem acesso} {saldo}
    o Eliminar um utilizador
    ▪ DEL {username}
    o Lista utilizadores
    ▪ LIST
    o Configura tempo de atualização do valor das ações geradas pelo servidor
    ▪ REFRESH {segundos}
    o Sair da consola
    ▪ QUIT
    o Desligar servidor
    ▪ QUIT_SERVER
*/

int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        printf("Usage: ./stock_server <PORT_ADMIN> <PORT_CLIENT> <config_file>\n");
        exit(-1);
    }

    int PORT = atoi(argv[1]);
    int PORT_ADMIN = atoi(argv[2]);
    char *config_file = argv[3];

    // read the database
    FILE *fp = fopen(config_file, "r");
    // check if something went wrong
    if (fp == NULL)
    {
        printf("Error opening file.\n");
        exit(-1);
    }

    /* pthread_t refresh;
    if (pthread_create(&refresh, NULL, pricesVolutality, NULL) != 0)
    {
        printf("\ncan't create thread\n");
        exit(-1);
    }
 */
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
        // if you fget only \n
        if (line[0] == '\n')
        {
            continue;
        }

        if (i == 0)
        {
            // user: User1;pass1;1000
            char *name = strtok(line, "/");
            char *pass = strtok(NULL, "");

            root->name = malloc(strlen(name) + 1);
            strcpy(root->name, name);
            root->password = malloc(strlen(pass) + 1);

            // replace last char with \0
            strcpy(root->password, pass);
            pass[strlen(pass) - 1] = '\0';
        }
        else if (i == 1)
        {
            // read line
            initial_users_number = atoi(line);

            // check if greater than 10, if it is throw errr
            if (initial_users_number > 10)
            {
                printf("Error: The number of users is greater than 10\n");
                return -1;
            }
        }
        else if (i >= 2 && i <= initial_users_number + 1)
        {
            int len = get_users_size(users_list);
            if (len >= 10)
            {
                printf("Error: The number of users is greater than 10\n");
                return -1;
            }

            // user: User1;pass1;1000;BINANCE;FTX
            char *name = strtok(line, ";");
            char *pass = strtok(NULL, ";");
            char *saldo = strtok(NULL, ";");
            char *bolsas = strtok(NULL, ";");
            char *bolsas2 = strtok(NULL, ";");

            // read line
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
                // remove \n from bolsas
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
                // remove \n from bolsas
                bolsas2[strlen(bolsas2) - 1] = '\0';
                user->bolsa2 = malloc(strlen(bolsas2) + 1);
                strcpy(user->bolsa2, bolsas2);
            }

            // append to the list
            append_user(users_list, user);
        }
        else
        {
            // read line
            char *market = strtok(line, ";");
            char *nomeacao = strtok(NULL, ";");
            char *preco = strtok(NULL, ";");

            struct Acao *acao = malloc(sizeof(struct Acao));
            acao->mercado = malloc(strlen(market) + 1);
            strcpy(acao->mercado, market);
            acao->nomestock = malloc(strlen(nomeacao) + 1);
            strcpy(acao->nomestock, nomeacao);

            acao->currentprice = atof(preco);
            // append to the list

            append_acao(acao_list, acao);
            i++;
        }

        i++;
    }
    // list the root user, all the normal users, and all the stocks
    printf("Root user: %s %s\n", root->name, root->password);

    printf("List of stocks:\n");
    list_stocks(acao_list);

    // print all the users
    printf("List of users:\n");
    list_users(users_list);

    // assign to shared the memory
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

    // create two processes, 1 for tcp and one for udp
    if (fork() == 0)
    {
        // tcp - clientes non admin 9876
        tcp_server(PORT, acao_list, users_list, root);
    }
    else if (fork() == 0)
    {
        // udp - clientes admin 9877
        udp_server(PORT_ADMIN, acao_list, users_list, root);
    }
    // parent process
    while (1)
    {
        wait(NULL);
    }

    /* pthread_join(refresh, NULL); */
    return 0;
}
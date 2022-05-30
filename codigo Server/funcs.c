#include "funcs.h"
pthread_mutex_t pricesmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t feedmutex = PTHREAD_MUTEX_INITIALIZER;
/*
{username administrador}/{password de administrador}
{Número de utilizadores iniciais} # no máximo 5
[username utilizador 1;password utilizador 1;saldo inicial]
(..)
{mercado};{ação};{preço inicial} # 2 mercados, cada um com 3 ações
(..)
*/

void clientep(int client_fd, struct AcaoList *acao_list, struct UsrList *users_list)
{
    int nread = 1;
    char buffer[BUF_SIZE];
    char *token;
    char username[BUF_SIZE];
    char password[BUF_SIZE];
    int out = 1;
    char inputsMenu[200] = "1 - Live Feed\n2 - Turn Off Feed\n3 - Subscribe\n4 - Show wallet contents\n5 - BUY\n6 - SELL\n7 - EXIT\n";
    struct NormalUser *user = malloc(sizeof(struct NormalUser));

    while (nread > 0)
    {
        send(client_fd, "AUTH", 5, 0);
        nread = read(client_fd, buffer, BUF_SIZE + 1);
        buffer[nread] = '\0';

        token = strtok(buffer, ";");
        strcpy(username, token);

        token = strtok(NULL, ";");
        strcpy(password, token);

        printf("username: %s\n", username);
        printf("password: %s\n", password);

        user = UserbyName(username, users_list);
        if (user == NULL)
        {
            printf("User not found\n");
            send(client_fd, "USER NOT FOUND", 15, 0);
            continue;
        }
        else if (strcmp(user->password, password) != 0)
        {
            printf("password incorrect\n");
            send(client_fd, "WRONG PASSWORD", 15, 0);
            continue;
        }
        else
        {
            printf("User logged in\n");
            send(client_fd, "OK", 3, 0);
            out = 0;
            break;
        }
        if (out == 1)
        {
            send(client_fd, "LOGIN FAILED, TRY AGAIN", 24, 0);
        }
        fflush(stdout);
    }
    pthread_t feed;
    struct threadinfo info_feed;
    info_feed.fd = client_fd;
    info_feed.user = user;
    int feed_status = 0;
    if (user->bolsa1 == NULL && user->bolsa2 == NULL)
    {
        send(client_fd, "ANY WALLET", 9, 0);
        close(client_fd);
        exit(0);
    }

    sleep(1);
    
    struct AcaoList *aux = acao_list;
    while (aux != NULL)
    {
        char acaoString[150] = "ACAO: ";
        char auxfloat[10];
        if (strcmp(user->bolsa1, aux->acao->mercado) == 0 || strcmp(user->bolsa2, aux->acao->mercado) == 0)
        {
            strcat(acaoString, aux->acao->nomestock);
            strcat(acaoString, " ");
            strcat(acaoString, aux->acao->mercado);
            strcat(acaoString, " ");
            strcat(acaoString, "PRICE: ");
            sprintf(auxfloat, "%f", aux->acao->currentprice);
            strcat(acaoString, auxfloat);
            strcat(acaoString, "\n");
            send(client_fd, acaoString, strlen(acaoString), 0);
        }
        aux = aux->next;
    }
    do
    {
        write(client_fd, inputsMenu, strlen(inputsMenu) + 1);
        nread = read(client_fd, buffer, BUF_SIZE + 1);
        buffer[nread] = '\0';

        if (strcmp(buffer, "1\n") == 0)
        {
            feed_status = 1;
            if (feed_status == 0)
            {
                pthread_create(&feed, NULL, &feed_thread, &info_feed);
            }
            else
            {
                write(client_fd, "FEED ALREADY ON", 15);
            }
        }
        else if (strcmp(buffer, "2\n") == 0)
        {
            if (feed_status == 1)
            {
                pthread_cancel(feed);
                write(client_fd, "FEED OFF", 8);
            }
            else
            {
                write(client_fd, "FEED ALREADY OFF", 16);
            }
            feed_status = 0;
        }
        else if (strcmp(buffer, "3") == 0)
        {
            char toSend[BUF_SIZE];
            strcpy(toSend, "Wallet Info:\n");
            strcat(toSend, "Current Money: ");

            char money[200];
            sprintf(money, "%d", user->saldo);
            strcat(toSend, money);
            strcat(toSend, "\n");
        }

        fflush(stdout);
    } while (nread > 0);
    pthread_join(feed, NULL);
    close(client_fd);

}

void tcp_server(int PORT_ADMIN, struct AcaoList *acao_list, struct UsrList *users_list, struct RootUser *root)
{
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT_ADMIN);
    fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (fd < 0)
        erro("in socket");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("in bind");
    if (listen(fd, 5) < 0)
        erro("in listen");
    client_addr_size = sizeof(client_addr);
    printf("[SERVER TCP] Started.\n");
    
    while (1)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
        if (client > 0)
        {
            printf("[SERVER TCP] Client Connected\n");
            if (fork() == 0)
            {
                close(fd);
                clientep(client, acao_list, users_list);
                exit(0);
            }
            close(client);
        }
    }
}

int udp_server(int PORT, struct AcaoList *acao_list, struct UsrList *users_list, struct RootUser *root)
{
    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char buf[BUFLEN];
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("It was not possible to create the socket");
    }
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
    {
        erro("Error in client bind");
    }
    printf("[SERVER UDP] Waiting for packets\n");
    while(1) {
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
        {
        erro("Erro no recvfrom");
        }
        buf[recv_len] = '\0';
        char *username;
        char *password;
        char *saldo;
        char *bolsas;
        char *bolsas2;
        printf("Client[%s:%d] %s\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port), buf);
        char *command = strtok(buf, " \n");
        if (strcmp(command, "ADD_USER") == 0)
        {
            username = strtok(NULL, " ");
            password = strtok(NULL, " ");
            saldo = strtok(NULL, " ");
            bolsas = strtok(NULL, " ");
            bolsas2 = strtok(NULL, " ");
            if (bolsas == NULL)
            {
                bolsas = malloc(1);
                bolsas[0] = '\0';
            }
            else
            { 
                bolsas[strlen(bolsas) - 1] = '\0';
            }
            if (bolsas2 == NULL)
            {
                bolsas2 = malloc(1);
                bolsas2[0] = '\0';
            }
            else
            {  
                bolsas2[strlen(bolsas2) - 1] = '\0';
            }
            if (user_exists(username, users_list))
            {
                sendto(s, "[SERVER] USER ALREADY EXISTS\n", strlen("[SERVER] USER ALREADY EXISTS\n"), 0, (struct sockaddr *)&si_outra, slen);
                continue;
            }

            struct NormalUser *user = malloc(sizeof(struct NormalUser));
            user->name = malloc(strlen(username) + 1);
            strcpy(user->name, username);
            user->password = malloc(strlen(password) + 1);
            strcpy(user->password, password);
            int saldo_int = atoi(saldo);
            if (saldo_int < 0)
            {
                sendto(s, "[SERVER] INVALID SALDO\n", strlen("[SERVER] INVALID SALDO\n"), 0, (struct sockaddr *)&si_outra, slen);
                return -1;
            }
            user->saldo = saldo_int;

            user->bolsa1 = malloc(strlen(bolsas) + 1);
            strcpy(user->bolsa1, bolsas);

            user->bolsa2 = malloc(strlen(bolsas2) + 1);
            strcpy(user->bolsa2, bolsas2);
            append_user(users_list, user);
            save_to_file();
        }
        
        else if (strcmp(command, "DEL") == 0)
        {
            username = strtok(NULL, " ");
            username[strlen(username) - 1] = '\0';
            if (!user_exists(username, users_list))
            {
                printf("User does not exist!\n");
                sendto(s, "[SERVER] USER DOES NOT EXIST\n", strlen("[SERVER] USER DOES NOT EXIST\n"), 0, (struct sockaddr *)&si_outra, slen);
                continue;
            }
            char jj[BUFLEN];
            sprintf(jj, "[SERVER] USER %s DELETED\n", username);
            sendto(s, jj, strlen(jj), 0, (struct sockaddr *)&si_outra, slen);
            delete_user(users_list, username);

            save_to_file();
        }
        else if (strcmp(command, "LIST") == 0)
        {
           list_users(users_list, s, &si_outra, slen);
        }
        else if (strcmp(command, "REFRESH") == 0)
        {
            char *segundos = strtok(NULL, " ");
            refresh_time(segundos,s, &si_outra, slen);
        }
        else if (strcmp(command, "QUIT") == 0)
        {
            printf("[CLIENT] Quitting...\n");
            if (sendto(s, "QUIT", strlen("QUIT"), 0, (struct sockaddr *)&si_outra, slen) == -1)
            {
                erro("Erro no sendto");
            }
            close(s);
            continue;
        }
        else if (strcmp(command, "QUIT_SERVER") == 0)
        { 
            printf("[SERVER] Quitting...\n");
            break;
        }
        else
        {
            printf("[CLIENT] Invalid command: %s\n", command);
            char invalid_command[BUFLEN];
            sprintf(invalid_command, "[SERVER] INVALID COMMAND %s\n", command);
            sendto(s, invalid_command, strlen(invalid_command), 0, (struct sockaddr *)&si_outra, slen);
        }
    }
    close(s);
    return 0;
}

void *feed_thread(void *arg)
{
    struct threadinfo treadinformation = *((struct threadinfo *)arg);
    int client_fd = treadinformation.fd;
    struct NormalUser *user = treadinformation.user;

    while (1)
    {
        pthread_mutex_lock(&feedmutex);
        sleep(shm->refresh_time);
        struct AcaoList *aux = shm->acao_list;
        while (aux != NULL)
        {
            char acaoString[150] = "ACAO: ";
            char auxfloat[10];
            if (strcmp(user->bolsa1, aux->acao->mercado) == 0 || strcmp(user->bolsa2, aux->acao->mercado) == 0)
            {
                strcat(acaoString, aux->acao->nomestock);
                strcat(acaoString, " ");
                strcat(acaoString, aux->acao->mercado);
                strcat(acaoString, " ");
                strcat(acaoString, "PRICE: ");
                sprintf(auxfloat, "%f", aux->acao->currentprice);
                strcat(acaoString, auxfloat);
                strcat(acaoString, "\n");
                send(client_fd, acaoString, strlen(acaoString), 0);
            }
            aux = aux->next;
        }
        pthread_mutex_unlock(&feedmutex);
    }
}

void erro(char *s)
{
    perror(s);
    exit(1);
}

struct NormalUser *UserbyName(char *username, struct UsrList *users_list)
{
    struct UsrList *aux = users_list->next;
    while (aux != NULL)
    {
        printf("%s\n", aux->user->name);
        if (strcmp(aux->user->name, username) == 0)
        {
            return aux->user;
        }
        aux = aux->next;
    }
    return NULL;
}

int userSize(struct UsrList *users_list)
{
    int i = 0;
    struct UsrList *aux = users_list;
    while (aux != NULL)
    {
        i++;
        aux = aux->next;
    }
    return i;
}


void append_user(struct UsrList *users_list, struct NormalUser *user)
{
    struct UsrList *aux = users_list;
    while (aux->next != NULL)
    {
        aux = aux->next;
    }
    aux->next = malloc(sizeof(struct UsrList));
    aux->next->user = user;
    aux->next->next = NULL;
}


struct NormalUser *get_user(struct UsrList *users_list, int index)
{
    int i = 0;
    struct UsrList *aux = users_list;
    while (aux != NULL)
    {
        if (i == index)
        {
            return aux->user;
        }
        i++;
        aux = aux->next;
    }
    return NULL;
}


void append_acao(struct AcaoList *acao_list, struct Acao *acao)
{
    struct AcaoList *new_acao = malloc(sizeof(struct AcaoList));
    new_acao->acao = acao;
    new_acao->next = NULL;

    if (acao_list->next == NULL)
    {
        acao_list->next = new_acao;
    }
    else
    {
        struct AcaoList *aux = acao_list->next;
        while (aux->next != NULL)
        {
            aux = aux->next;
        }
        aux->next = new_acao;
    }
}

int user_exists(char *username, struct UsrList *users_list)
{
    struct UsrList *aux = users_list->next;
    while (aux != NULL)
    {
        if (strcmp(aux->user->name, username) == 0)
        {
            return 1;
        }
        aux = aux->next;
    }
    return 0;
}

void refresh_time(char *segundos, int socket_fd, struct sockaddr_in *cli_addr, socklen_t slen )
{
    shm->refresh_time = atoi(segundos);

    printf("Refresh time set to %d seconds\n", shm->refresh_time);
    char finalstring[100] = "Refresh time set to ";
    strcat(finalstring, segundos);
    strcat(finalstring, "\n");
    sendto(socket_fd, finalstring, strlen(finalstring), 0, (struct sockaddr *)cli_addr, slen);
}

void delete_user(struct UsrList *users_list, char *username)
{
    struct UsrList *aux = users_list;
    while (aux->next != NULL)
    {
        if (strcmp(aux->next->user->name, username) == 0)
        {
            struct UsrList *aux2 = aux->next;
            aux->next = aux->next->next;
            free(aux2);
            return;
        }
        aux = aux->next;
    }
}

void list_users(struct UsrList *users_list, int socket_fd, struct sockaddr_in *cli_addr, socklen_t slen )

{
    struct UsrList *aux = users_list->next;
        char finalstring[6000] = "";
    while (aux != NULL)
    {
        if (aux->user->bolsa1 == NULL)
        {
            printf("%s - %d\n", aux->user->name, aux->user->saldo);
            strcat(finalstring, aux->user->name);
            strcat(finalstring, " - ");
            char saldo[10];
            sprintf(saldo, "%d", aux->user->saldo);
            strcat(finalstring, saldo);
            strcat(finalstring, "\n");
        }
        else
        {
            if (aux->user->bolsa2 == NULL)
            {

                printf("%s - %d - %s\n", aux->user->name, aux->user->saldo, aux->user->bolsa1);
                strcat(finalstring, aux->user->name);
                strcat(finalstring, " - ");
                char saldo[10];
                sprintf(saldo, "%d", aux->user->saldo);
                strcat(finalstring, saldo);
                strcat(finalstring, " - ");
                strcat(finalstring, aux->user->bolsa1);
                strcat(finalstring, "\n");

            }
            else
            {
                printf("%s - %d - %s - %s\n", aux->user->name, aux->user->saldo, aux->user->bolsa1, aux->user->bolsa2);
                strcat(finalstring, aux->user->name);
                strcat(finalstring, " - ");
                char saldo[10];
                sprintf(saldo, "%d", aux->user->saldo);
                strcat(finalstring, saldo);
                strcat(finalstring, " - ");
                strcat(finalstring, aux->user->bolsa1);
                strcat(finalstring, " - ");
                strcat(finalstring, aux->user->bolsa2);
                strcat(finalstring, "\n");
            }
        }
        aux = aux->next;
    }
    sendto(socket_fd, finalstring, strlen(finalstring), 0, (struct sockaddr *)cli_addr, slen);
}

int get_users_lenght(struct UsrList *users_list)
{
    int i = 0;
    struct UsrList *aux = users_list->next;
    while (aux != NULL)
    {
        i++;
        aux = aux->next;
    }
    return i;
}

void save_to_file()
{
    sem_wait(&shm->sem_write);
    struct UsrList *users_list = shm->users_list;
    struct AcaoList *acao_list = shm->acao_list;
    struct RootUser *root_user = shm->root;
    FILE *fp = fopen("database.txt", "w");
    char *together = malloc(sizeof(char) * 500);
    memset(together, 0, 500); 

    char *finalstring = malloc(sizeof(char) * 3000);
    memset(finalstring, 0, 3000); 
    sprintf(together, "%s/%s", root_user->name, root_user->password);

    strcat(finalstring, together);
    int users_lenght = get_users_lenght(users_list);
    sprintf(together, "%d\n", users_lenght);
    strcat(finalstring, together);

    struct UsrList *aux = users_list->next;
    while (aux != NULL)
    {
        if (aux->user->bolsa1 != NULL && strlen(aux->user->bolsa1) > 0)
        {
            if (aux->user->bolsa2 != NULL && strlen(aux->user->bolsa2) > 0)
            {
                sprintf(together, "%s;%s;%d;%s;%s\n", aux->user->name, aux->user->password, aux->user->saldo, aux->user->bolsa1, aux->user->bolsa2);
            }
            else
            {
                sprintf(together, "%s;%s;%d;%s\n", aux->user->name, aux->user->password, aux->user->saldo, aux->user->bolsa1);
            }
        }
        else
        {
            sprintf(together, "%s;%s;%d\n", aux->user->name, aux->user->password, aux->user->saldo);
        }
        

        strcat(finalstring, together);
        aux = aux->next;
    }
    struct AcaoList *aux2 = acao_list->next;
    while (aux2 != NULL)
    {
        sprintf(together, "%s;%s;%f\n", aux2->acao->mercado, aux2->acao->nomestock, aux2->acao->currentprice);
        strcat(finalstring, together);
        aux2 = aux2->next;
    }
    strcat(finalstring, "\0");
    int i = 0;
    while (finalstring[i] != '\0')
    {
        fputc(finalstring[i], fp);
        i++;
    }

    sem_post(&shm->sem_write);
    fflush(fp);
    fclose(fp);
}



void menu(){
    
    int choice =-1;
    while(choice!= 7){
        printf("1 - Live Feed\n2 - Turn Off Feed\n3 - Subscribe\n4 - Show wallet contents\n5 - BUY\n6 - SELL\n7 - EXIT\n");
        scanf("%d", &choice);
        if (choice == 1)
        {
            printf("Live Feed\n");
        }
        else if (choice == 2)
        {
            printf("Turn Off Feed\n");
        }
        else if (choice == 3)
        {
            printf("Subscribe\n");
        }
        else if (choice == 4)
        {
            printf("Show wallet contents\n");
        }
        else if (choice == 5)
        {
            printf("BUY\n");
        }
        else if (choice == 6)
        {
            printf("SELL\n");
        }
         
    }
}
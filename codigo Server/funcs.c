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

void process_client(int client_fd, struct AcaoList *acao_list, struct UsrList *users_list)
{
    int nread = 1;
    char buffer[BUF_SIZE];
    char *message = malloc(sizeof(char) * BUF_SIZE + 1);
    char *token;
    char username[BUF_SIZE];
    char password[BUF_SIZE];
    int out = 1;
    char inputsMenu[200] = "1 - Live Feed\n2 - Turn Off Feed\n3 - Subscribe\n4 - Show wallet contents\n5 - BUY\n6 - SELL\n7 - EXIT\n";

    // save the user logged in
    struct NormalUser *user = malloc(sizeof(struct NormalUser));

    while (nread > 0)
    {

        // send to the client asking for auth
        send(client_fd, "AUTH", 5, 0);
        // receive the answer
        nread = read(client_fd, buffer, BUF_SIZE + 1);
        buffer[nread] = '\0';

        // split buffer with ;
        token = strtok(buffer, ";");
        strcpy(username, token);

        token = strtok(NULL, ";");
        strcpy(password, token);

        printf("username: %s\n", username);
        printf("password: %s\n", password);

        // list_users(users_list);

        user = get_user_by_name(username, users_list);
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

    // check the user's bolsas
    if (user->bolsa1 == NULL && user->bolsa2 == NULL)
    {
        send(client_fd, "NO BOLSAS", 9, 0);
        close(client_fd);
        exit(0);
    }

    sleep(1);

    // loop through the acoes in the shared memory
    struct AcaoList *aux = acao_list;
    while (aux != NULL)
    {
        char info_Acao[150] = "ACAO: ";
        char auxfloat[10];
        // check if the user is subscribed to the acao
        if (strcmp(user->bolsa1, aux->acao->mercado) == 0 || strcmp(user->bolsa2, aux->acao->mercado) == 0)
        {
            strcat(info_Acao, aux->acao->nomestock);
            strcat(info_Acao, " ");
            strcat(info_Acao, aux->acao->mercado);
            strcat(info_Acao, " ");
            strcat(info_Acao, "PRICE: ");
            sprintf(auxfloat, "%f", aux->acao->currentprice);
            strcat(info_Acao, auxfloat);
            strcat(info_Acao, "\n");
            send(client_fd, info_Acao, strlen(info_Acao), 0);
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
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind");
    if (listen(fd, 5) < 0)
        erro("na funcao listen");
    client_addr_size = sizeof(client_addr);
    printf("[SERVER TCP] Started.\n");
    while (1)
    {
        // clean finished child processes, avoiding zombies
        // must use WNOHANG or would block whenever a child process was working
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        // wait for new connection
        client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
        if (client > 0)
        {
            printf("[SERVER TCP] Client Connected\n");
            if (fork() == 0)
            {
                close(fd);
                process_client(client, acao_list, users_list);
                exit(0);
            }
            close(client);
        }
    }
}

int udp_server(int PORT, struct AcaoList *acao_list, struct UsrList *users_list, struct RootUser *root)
{

    // server start:
    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char buf[BUFLEN];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("Erro na criação do socket do cliente.");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = PF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
    {
        erro("Erro no bind do cliente.");
    }

    char *username;
    char *password;
    char *saldo;
    char *bolsas;
    char *bolsas2;

    char cChar;

    printf("[SERVER UDP] Waiting for packets\n");

    sendto(s, "Please Authenticate in this format: \"username\";\"password\"", 19, 0, (struct sockaddr *)&si_outra, slen);
    // Espera recepção de mensagem (a chamada é bloqueante)
    if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
    {
        erro("[CLIENT] Erro no recvfrom");
    }
    // Para ignorar o restante conteúdo (anterior do buffer)
    buf[recv_len] = '\0';

    // ask for authentication (if not, send error message)
    if (strcmp(buf, "AUTH") == 0)
    {
        printf("[SERVER UDP] AUTH\n");
        // send to the client asking for auth
        sendto(s, "AUTH", 5, 0, (struct sockaddr *)&si_outra, slen);
        // receive the answer
        recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen);
        buf[recv_len] = '\0';
        // split buffer with ;
        char *token;
        token = strtok(buf, ";");
        username = token;
        token = strtok(NULL, ";");
        password = token;
    }
    else
    {
        printf("[SERVER UDP] AUTH FAILED\n");
        sendto(s, "AUTH FAILED", 12, 0, (struct sockaddr *)&si_outra, slen);
        return 0;
    }

    if (check_valid_admin_cred(root, username, password))
    {
        // send OK
        sendto(s, "OK", 2, 0, (struct sockaddr *)&si_outra, slen);
    }
    else
    {
        // send FAIL
        sendto(s, "FAIL", 4, 0, (struct sockaddr *)&si_outra, slen);
    }

    while (1)
    {

        // Espera recepção de mensagem (a chamada é bloqueante)
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1)
        {
            erro("[CLIENT] Erro no recvfrom");
        }

        // Para ignorar o restante conteúdo (anterior do buffer)
        buf[recv_len] = '\0';

        if (strcmp(buf, "X") == 0)
        {
            continue;
        }

        printf("Client[%s:%d] %s\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port), buf);

        char *command = strtok(buf, " \n");

        // comand has this format ADD_USER {username} {password} {saldo} {bolsa1} {bolsa2}
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
                // remove \n from bolsas and replace it with \0
                bolsas[strlen(bolsas) - 1] = '\0';
            }

            if (bolsas2 == NULL)
            {
                bolsas2 = malloc(1);
                bolsas2[0] = '\0';
            }
            else
            {
                // remove \n from bolsas2 and replace it with \0
                bolsas2[strlen(bolsas2) - 1] = '\0';
            }

            // printf("%s %s %s %s\n", username, password, saldo, bolsas);

            if (user_exists(username, users_list))
            {
                printf("[CLIENT] User already exists!\n");
                continue;
            }

            // printf("User: %s Password: %s Saldo: %s\n", username, password, saldo);

            struct NormalUser *user = malloc(sizeof(struct NormalUser));
            user->name = malloc(strlen(username) + 1);
            strcpy(user->name, username);
            user->password = malloc(strlen(password) + 1);
            strcpy(user->password, password);
            int saldo_int = atoi(saldo);
            if (saldo_int < 0)
            {
                printf("[CLIENT] The saldo is invalid!\n");
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
        // comand has this format DEL {username}
        else if (strcmp(command, "DEL") == 0)
        {
            username = strtok(NULL, " ");
            // find \n and replace it with \0
            username[strlen(username) - 1] = '\0';

            if (!user_exists(username, users_list))
            {
                printf("User does not exist!\n");
                continue;
            }
            printf("[CLIENT] Deleting User: %s\n", username);
            delete_user(users_list, username);

            save_to_file();
        }
        // comand has this format LIST
        else if (strcmp(command, "LIST") == 0)
        {
            list_users(users_list);
        }
        // comand has this format REFRESH {segundos}
        else if (strcmp(command, "REFRESH") == 0)
        {
            char *segundos = strtok(NULL, " ");
            refresh_time(segundos);
        }
        // comand has this format QUIT
        else if (strcmp(command, "QUIT") == 0)
        {
            printf("[CLIENT] Quitting...\n");
            // DISCONNECT CLIENT
            // send QUIT to client
            if (sendto(s, "QUIT", strlen("QUIT"), 0, (struct sockaddr *)&si_outra, slen) == -1)
            {
                erro("Erro no sendto");
            }
            break;
        }
        else if (strcmp(command, "QUIT_SERVER") == 0)
        { // qual é a diferença, SAIR DO SERVER
            return 1;
        }
        else
        {
            printf("[CLIENT] Invalid command: %s\n", command);
        }
    }
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

        // loop through the acoes in the shared memory
        struct AcaoList *aux = shm->acao_list;
        while (aux != NULL)
        {
            char info_Acao[150] = "ACAO: ";
            char auxfloat[10];
            // check if the user is subscribed to the acao
            if (strcmp(user->bolsa1, aux->acao->mercado) == 0 || strcmp(user->bolsa2, aux->acao->mercado) == 0)
            {
                strcat(info_Acao, aux->acao->nomestock);
                strcat(info_Acao, " ");
                strcat(info_Acao, aux->acao->mercado);
                strcat(info_Acao, " ");
                strcat(info_Acao, "PRICE: ");
                sprintf(auxfloat, "%f", aux->acao->currentprice);
                strcat(info_Acao, auxfloat);
                strcat(info_Acao, "\n");
                send(client_fd, info_Acao, strlen(info_Acao), 0);
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

int get_number_of_users()
{
    // check the second line of the database
    FILE *fp = fopen("database.txt", "r");
    char *line = NULL;
    size_t len = 0;

    int i = 0;
    while (fgets(line, len, fp) != NULL)
    {
        if (i == 1)
        {
            return atoi(line);
        }
        i++;
    }
    return 0;
}
// get user
struct NormalUser *get_user_by_name(char *username, struct UsrList *users_list)
{
    struct UsrList *aux = users_list->next;
    printf("[CLIENT] Getting user by name: %s\n", username);
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

int get_users_size(struct UsrList *users_list)
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

int get_acao_size(struct AcaoList *acao_list)
{
    int i = 0;
    struct AcaoList *aux = acao_list;
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

struct Acao *get_acao(struct AcaoList *acao_list, int index)
{
    int i = 0;
    struct AcaoList *aux = acao_list;
    while (aux != NULL)
    {
        if (i == index)
        {
            return aux->acao;
        }
        i++;
        aux = aux->next;
    }
    return NULL;
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

void *pricesVolutality()
{
    while (1)
    {
        pthread_mutex_lock(&pricesmutex);
        sleep(shm->refresh_time);
        // loop through acoes
        struct AcaoList *aux = shm->acao_list;
        while (aux != NULL)
        {
            // variate the prices of the acoes by
            // a random number between -5 and 5
            int variacao = rand() % 10 - 5;
            aux->acao->currentprice = aux->acao->currentprice + variacao;
            aux = aux->next;
        }

        save_to_file();

        pthread_mutex_unlock(&pricesmutex);
    }
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

void refresh_time(char *segundos)
{
    // write the user to the database
    // parse seconds to int

    // set the shared memory to the new time
    shm->refresh_time = atoi(segundos);

    printf("Refresh time set to %d seconds\n", shm->refresh_time);
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

void list_users(struct UsrList *users_list)
{
    struct UsrList *aux = users_list->next;
    while (aux != NULL)
    {
        if (aux->user->bolsa1 == NULL)
        {
            printf("%s - %d\n", aux->user->name, aux->user->saldo);
        }
        else
        {
            if (aux->user->bolsa2 == NULL)
            {
                printf("%s - %d - %s\n", aux->user->name, aux->user->saldo, aux->user->bolsa1);
            }
            else
            {
                printf("%s - %d - %s - %s\n", aux->user->name, aux->user->saldo, aux->user->bolsa1, aux->user->bolsa2);
            }
        }
        aux = aux->next;
    }
}

void list_stocks(struct AcaoList *acao_list)
{
    struct AcaoList *aux = acao_list->next;
    while (aux != NULL)
    {
        printf("%s - %s @ %f\n", aux->acao->mercado, aux->acao->nomestock, aux->acao->currentprice);
        aux = aux->next;
    }
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
    // check if database has something inside, if it does delete it
    FILE *fp = fopen("database.txt", "w");
    char *together = malloc(sizeof(char) * 500);
    memset(together, 0, 500); //! important

    char *toprint = malloc(sizeof(char) * 3000);
    memset(toprint, 0, 3000); //! important

    // write root user:
    sprintf(together, "%s/%s", root_user->name, root_user->password);

    strcat(toprint, together);

    // write users:
    // get users lenght
    int users_lenght = get_users_lenght(users_list);
    sprintf(together, "%d\n", users_lenght);
    strcat(toprint, together);

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
        //  add \n to the end of the string

        strcat(toprint, together);
        aux = aux->next;
    }
    // printf("%s\n", toprint);

    // write acao_list:
    struct AcaoList *aux2 = acao_list->next;
    while (aux2 != NULL)
    {
        sprintf(together, "%s;%s;%f\n", aux2->acao->mercado, aux2->acao->nomestock, aux2->acao->currentprice);
        strcat(toprint, together);
        aux2 = aux2->next;
    }
    // add \0 at the end of toprint
    strcat(toprint, "\0");

    // for each char in toprint, put in database.txt
    int i = 0;
    while (toprint[i] != '\0')
    {
        // printf("%c", toprint[i]);
        fputc(toprint[i], fp);
        i++;
    }

    sem_post(&shm->sem_write);
    // close file
    fflush(fp);
    fclose(fp);
}

int check_valid_admin_cred(struct RootUser *root_user, char *name, char *password)
{
    return strcmp(root_user->name, name) == 0 && strcmp(root_user->password, password) == 0;
}
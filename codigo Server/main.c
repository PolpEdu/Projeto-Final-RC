#include "funcs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
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
    printf("Starting the Server on PORT: %d\nInitializing Database.\n", PORT);
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

    // read the database
    FILE *fp = fopen("database.txt", "r");
    char line[256];

    int i = 0;
    while (fgets(line, sizeof(line), fp))
    {
        //if you fget only \n
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
            strcpy(root->password, pass);
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

            // user: User1;pass1;1000
            char *name = strtok(line, ";");
            char *pass = strtok(NULL, ";");
            char *saldo = strtok(NULL, ";");
            char *bolsas = strtok(NULL, ";");
            

            // read line
            struct NormalUser *user = malloc(sizeof(struct NormalUser));
            user->name = malloc(strlen(name) + 1);
            strcpy(user->name, name);

            user->password = malloc(strlen(pass) + 1);
            strcpy(user->password, pass);
            user->saldo = atoi(saldo);

            if (bolsas == NULL)
            {
                user->bolsa = NULL;
            }
            else
            {
                //remove \n from bolsas
                bolsas[strlen(bolsas) - 1] = '\0';
                user->bolsa = malloc(strlen(bolsas) + 1);
                strcpy(user->bolsa, bolsas);
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
    /*
        // list the root user, all the normal users, and all the stocks
        printf("Root user: %s\n", root->name);

        printf("List of stocks:\n");
        list_stocks(acao_list);

        // print all the users
        printf("List of users:\n");
        list_users(users_list);
    */

    // server start:
    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char buf[BUFLEN];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
    {
        erro("Erro no bind");
    }

    char *username;
    char *password;
    char *saldo;
    char *bolsas;

    char cChar;
    printf("Server waiting for packets \n");

    while (1) {

        // Espera recepção de mensagem (a chamada é bloqueante)
        if((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        
        // Para ignorar o restante conteúdo (anterior do buffer)
        buf[recv_len]='\0';

        if(strcmp(buf, "X")==0) {
            continue;
        }

        printf("[%s:%d] %s\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port),buf);

        
        char *command = strtok(buf, " \n");


        // comand has this format ADD_USER {username} {password} {saldo} {bolsas}
        if (strcmp(command, "ADD_USER") == 0)
        {

            username = strtok(NULL, " ");
            password = strtok(NULL, " ");
            saldo = strtok(NULL, " ");
            bolsas = strtok(NULL, " ");


            if (bolsas == NULL)
            {
                bolsas = malloc(1);
                bolsas[0] = '\0';
            }
            else{
                // remove \n from bolsas and replace it with \0
                bolsas[strlen(bolsas)-1] = '\0';

            }

            //printf("%s %s %s %s\n", username, password, saldo, bolsas);

            if (user_exists(username, users_list))
            {
                printf("User already exists!\n");
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
                printf("The saldo is invalid!\n");
                return -1;
            }
            user->saldo = saldo_int;

            user->bolsa = malloc(strlen(bolsas) + 1);
            strcpy(user->bolsa, bolsas);


            append_user(users_list, user);

            save_to_file(users_list, acao_list, root);
        }
        // comand has this format DEL {username}
        else if (strcmp(command, "DEL") == 0)
        {
            username = strtok(NULL, " ");
            //find \n and replace it with \0
            username[strlen(username)-1] = '\0';
            

            if (!user_exists(username, users_list))
            {
                printf("User does not exist!\n");
                continue;
            }
            printf("Deleting User: %s\n", username);
            delete_user(users_list, username);

            save_to_file(users_list, acao_list, root);
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
            break;
        }
        else if (strcmp(command, "QUIT_SERVER") == 0)
        { // qual é a diferença?
            return 1;
        }
        else
        {
            printf("Invalid command: %s\n", command);
        }
    }
    return 0;
}
#include "funcs.h"
#include <stdio.h>
#include <errno.h>
/*
{username administrador}/{password de administrador}
{Número de utilizadores iniciais} # no máximo 5
[username utilizador 1;password utilizador 1;saldo inicial]
(..)
{mercado};{ação};{preço inicial} # 2 mercados, cada um com 3 ações
(..)
*/

/*
    admin/admin_password
    2
    User1;pass1;1000
    User2;pass2;1500
    bvl;stock_bvl_1;10
    bvl;stock_bvl_2;10
    bvl;stock_bvl_3;10
    nyse;stock_nyse_1;10
    nyse;stock_nyse_2;10
    nyse;stock_nyse_3;10
*/

int REFRESH_TIME;


void erro(char *s) {
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
    REFRESH_TIME = atoi(segundos);
    printf("Refresh time set to %d seconds\n", REFRESH_TIME);
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
        if (aux->user->bolsa == NULL)
        {
            printf("%s - %d\n", aux->user->name, aux->user->saldo);
        }
        else
        {
            printf("%s - %d - %s\n", aux->user->name, aux->user->saldo, aux->user->bolsa);
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
void save_to_file(struct UsrList *users_list, struct AcaoList *acao_list, struct RootUser *root_user)
{
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
        if (aux->user->bolsa != NULL && strlen(aux->user->bolsa) > 0)
        {
            sprintf(together, "%s;%s;%d;%s\n", aux->user->name, aux->user->password, aux->user->saldo, aux->user->bolsa);
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

    // close file
    fflush(fp);
    fclose(fp);
}

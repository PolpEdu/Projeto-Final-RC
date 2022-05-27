#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#define BUFLEN 512 // Tamanho do buffer

void erro(char *msg);

int main(int argc, char *argv[])
{
  char endServer[100];
  int fd;
  struct sockaddr_in addr;
  struct hostent *hostPtr;

  if (argc != 3)
  {
    printf("./operations_terminal <host> <port>\n");
    exit(-1);
  }

  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Não consegui obter endereço");

  bzero((void *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short)atoi(argv[2]));

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    erro("socket");

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("connect");
  }

  // wait for server to send data
  char buffer[BUFLEN];
  char input[BUFLEN];
  char inputsMenu[200] = "1 - Live Feed\n2 - Turn Off Feed\n3 - Subscribe\n4 - Show wallet contents\n5 - BUY\n6 - SELL\n7 - EXIT\n";
  int n;
  bzero(buffer, BUFLEN);
  n = read(fd, buffer, BUFLEN);
  if (n < 0)
    erro("ERROR reading from socket");
  printf("%s\n", buffer);

  // check if server asked for authentication
  if (strcmp(buffer, "AUTH") == 0)
  {

    while (1)
    {
      char username[BUFLEN];
      char password[BUFLEN];
      // printf("Authentication required\n");
      printf("Username: ");
      bzero(username, BUFLEN);
      fgets(username, BUFLEN + 1, stdin);
      // replace the last charater with \0
      username[strlen(username) - 1] = '\0';

      printf("Password: ");
      bzero(password, BUFLEN);
      fgets(password, BUFLEN + 1, stdin);
      // replace the last charater with \0
      password[strlen(password) - 1] = '\0';

      // send to the server the username and password, create a string like this "username;password"
      char auth[BUFLEN];
      bzero(auth, BUFLEN);
      strcat(auth, username);
      strcat(auth, ";");
      strcat(auth, password);
      printf("%s\n", auth);
      n = write(fd, auth, strlen(auth));
      if (n < 0)
        erro("ERROR writing to socket");

      // wait for server to send data
      bzero(buffer, BUFLEN);
      n = read(fd, buffer, BUFLEN);
      if (n < 0)
        erro("ERROR reading from socket");

      printf("%s\n", buffer);

      // check if authentication was successful
      if (strcmp(buffer, "OK") == 0)
      {
        printf("Authentication successful\n");
        break;
      }
      else
      {
        printf("Login unssucessful. Please Try Again:\n");
      }
    }
  }
  else
  {
    printf("%s\n", buffer);
    printf("Authentication not required\n");
    exit(-1);
  }

  // check if server asked for the list of users
  // TODO: isto sai bugoso, dar fix:
  do
  {
    n = read(fd, buffer, BUFLEN);
    if (n < 0)
      erro("ERROR reading from socket");
    buffer[n] = '\0';
    printf("%s\n", buffer);

  } while (strcmp(buffer, inputsMenu) != 0);

  while (1)
  {

    fgets(input, BUFLEN + 1, stdin);
    // replace the last charater with \0
    input[strlen(input) - 1] = '\0';
    write(fd, input, strlen(input) + 1);

    n = read(fd, buffer, BUFLEN - 1);
    buffer[n] = '\0';
    printf("%s\n", buffer);

    if (strcmp(input, "3") == 0)
    {
      fgets(input, BUFLEN + 1, stdin);
      input[strlen(input) - 1] = '\0';

      write(fd, input, 1 + strlen(input));
    }
    else if (strcmp(input, "4") == 0)
    {
      write(fd, buffer, strlen(buffer) + 1);
    }

    n = read(fd, buffer, BUFLEN - 1);
    buffer[n] = '\0';
    printf("%s\n", buffer);
    fflush(stdout);
  }

  close(fd);
  exit(0);
}

void erro(char *msg)
{
  printf("Erro: %s\n", msg);
  exit(-1);
}

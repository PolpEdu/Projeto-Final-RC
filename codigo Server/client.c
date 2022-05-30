#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#define BUFLEN 512 

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
  char buffer[BUFLEN];
  char input[BUFLEN];
  int n;
  bzero(buffer, BUFLEN);
  n = read(fd, buffer, BUFLEN);
  if (n < 0)
    erro("ERROR reading from socket");
  printf("%s\n", buffer);

  if (strcmp(buffer, "AUTH") == 0)
  {

    while (1)
    {
      char username[BUFLEN];
      char password[BUFLEN];
      
      printf("Username: ");
      bzero(username, BUFLEN);
      fgets(username, BUFLEN + 1, stdin);
      
      username[strlen(username) - 1] = '\0';

      printf("Password: ");
      bzero(password, BUFLEN);
      fgets(password, BUFLEN + 1, stdin);
      
      password[strlen(password) - 1] = '\0';
      char auth[BUFLEN];
      bzero(auth, BUFLEN);
      strcat(auth, username);
      strcat(auth, ";");
      strcat(auth, password);
      printf("%s\n", auth);
      n = write(fd, auth, strlen(auth));
      if (n < 0)
        erro("ERROR writing to socket");
      bzero(buffer, BUFLEN);
      n = read(fd, buffer, BUFLEN);
      if (n < 0)
        erro("ERROR reading from socket");

      printf("%s\n", buffer);

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
  menu();

  close(fd);
  exit(0);
}


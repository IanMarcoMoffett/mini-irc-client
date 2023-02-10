#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_MSG_LENGTH 512


int client_sockfd = 0;
int sockfd = 0;

/*
 *  Cleans up the program after
 *  exit.
 */

void
cleanup(int dmmy)
{
  (void)dmmy;

  if (client_sockfd != 0)
  {
    close(client_sockfd);
  }

  exit(0);
}

void
send_command(char* cmd)
{
  send(sockfd, cmd, strlen(cmd), 0);
}

void
handle_input(const char* channel)
{
  char buf[256];                  /* User input buffer */
  char cmdbuf[MAX_MSG_LENGTH];    /* Command buffer */
  pid_t child = fork();           /* To prevent blocking reads from stdin */

  if (child == 0)
  {
    while (1)
    {
      read(STDIN_FILENO, buf, sizeof(buf));

      snprintf(cmdbuf, sizeof(cmdbuf), "PRIVMSG #%s :%s\r\n", channel, buf);
      send_command(cmdbuf);
      
      printf("\nMessage sent!\n");
    }
  }
}

int
connect_and_run(char ip[INET6_ADDRSTRLEN], const char* nick,
                const char* channel)
{
  char buf[1024];
  char cmdbuf[MAX_MSG_LENGTH];
  struct sockaddr_in server_addr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("Error: Could not open socket.\n");
    return sockfd;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(6667);

  if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
  {
    printf("Error: Invalid address \"%s\"\n", ip);
    return 1;
  }
  
  client_sockfd = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

  if (client_sockfd < 0)
  {
    printf("Error: Connection failed.\n");
    return 1;
  }  
  
  snprintf(cmdbuf, sizeof(cmdbuf), "NICK %s\r\n", nick);
  send_command(cmdbuf);

  snprintf(cmdbuf, sizeof(cmdbuf), "USER %s 0 * :%s\r\n", nick, nick);
  send_command(cmdbuf);

  snprintf(cmdbuf, sizeof(cmdbuf), "JOIN #%s\r\n", channel);
  send_command(cmdbuf);
  
  handle_input(channel);
  while (read(sockfd, buf, sizeof(buf)) != 0)
  {
    printf("%s\n", buf);
    memset(buf, 0, sizeof(buf));
  }
  
  return 0;
}

int
startup(const char* server, const char* nick, const char* channel)
{
  struct addrinfo hints;
  struct addrinfo* res;
  int status;
  char ipstr[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(server, NULL, &hints, &res)) != 0)
  {
    printf("Error: Could not connect to %s\n", server);
    return status;
  }
  
  /* Find an IPv4 address */
  struct addrinfo* p;
  for (p = res; p != NULL; p = p->ai_next)
  {
    void* addr;

    if (p->ai_family == AF_INET)
    {
      struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
      addr = &(ipv4->sin_addr);

      inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
      break;
    }
  }

  freeaddrinfo(res);
  connect_and_run(ipstr, nick, channel);
  return 0;
}

int
main(int argc, char** argv)
{
  if (argc < 4)
  {
    printf("Usage: %s <irc-server> <nick> <channel>\n", argv[0]);
    return 1;
  }
  
  signal(SIGTERM, cleanup);
  signal(SIGINT, cleanup);
  startup(argv[1], argv[2], argv[3]);

  return 0;
}

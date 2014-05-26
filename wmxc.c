#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifdef linux
#include "Portable.h"
#endif

#define CONFIG_DEFAULT_LISTENER "localhost"
#define CONFIG_DEFAULT_PORT "6999"

#define BUFF_SIZE 1024	/* reasonable size for a buffer */
#define HOST_SIZE 65	/* maximum size for a hostname is 64+null byte, also use for port */

void usage(char *progname) {
	fprintf(stderr, "usage: %s [-p port] [-q] command [args]\n", progname);
	exit(1);
}

// TODO: move fancy strings to wmxc, keeping remote protocol slim
int main(int argc, char **argv) {
	struct addrinfo hint, *r;
	struct timeval t;
	int fd;

	char	msg_buf	[BUFF_SIZE] = "";
	char	recv_buf[BUFF_SIZE] = "";
	char	send_buf[BUFF_SIZE] = "";

	char	addr[HOST_SIZE]	= CONFIG_DEFAULT_LISTENER;
	char	port[HOST_SIZE]	= CONFIG_DEFAULT_PORT;
	int	quiet	= 0;

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	t.tv_sec = 0;
	t.tv_usec = 10;

	{
		int i, c;
		while((c = getopt(argc, argv, "p:q")) != -1 ) {
			switch (c) {
				case 'p':
					strlcpy(port, optarg, HOST_SIZE);
					break;
				case 'q':
					quiet++;
					break;
				case '?':
				default:
					usage(argv[0]);
			}
		}
		if (optind == argc) usage(argv[0]);

		for (i = optind; i < argc; i++) {
			strlcat(send_buf, argv[i], BUFF_SIZE);
			strlcat(send_buf, " ", BUFF_SIZE);
		}
		send_buf[strlen(send_buf) - 1] = '\n'; // newln at end
	}

	getaddrinfo(addr, port, &hint, &r);
	fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
	// set a timeout, otherwise silent functions hang
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&t, sizeof(t));
	connect(fd, r->ai_addr, r->ai_addrlen);

	recv(fd, msg_buf, BUFF_SIZE, 0); // get rid of the banner

	send(fd, send_buf, strlen(send_buf), 0);
	if (!quiet) {
		while (recv(fd, recv_buf, BUFF_SIZE, 0) > 0) {
			printf(recv_buf);
		}
	}

	send(fd, "quit\n", 6, 0); // close it

	close(fd);
	return 0;
}

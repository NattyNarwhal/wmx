#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "Portable.h"

#define CONFIG_DEFAULT_LISTENER "localhost"
#define CONFIG_DEFAULT_PORT "6999"

#define BUFF_SIZE 1024

void usage(char *progname) {
	fprintf(stderr, "usage: %s [-p port] [-q] command [args]\n", progname);
	exit(1);
}

int main(int argc, char **argv) {
	struct addrinfo hint, *r;
	struct timeval t;
	int fd;

	char	msg_buf	[BUFF_SIZE] = "";
	char	recv_buf[BUFF_SIZE] = "";
	char	send_buf[BUFF_SIZE] = "";

	char	*addr	= CONFIG_DEFAULT_LISTENER;
	char	*port	= CONFIG_DEFAULT_PORT;
	int	quiet	= 0;

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	t.tv_sec = 0;
	t.tv_usec = 10;

	{
		int i, c;
		//char *temp = (char*)calloc(BUFF_SIZE, sizeof(char*));
		char temp[BUFF_SIZE] = "";
		while((c = getopt(argc, argv, "p:q")) != -1 ) {
			switch (c) {
				case 'p':
					port = optarg;
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
			strlcat(temp, argv[i], BUFF_SIZE);
			strlcat(temp, " ", BUFF_SIZE);
		}
		temp[strlen(temp) - 1] = '\n'; // newln at end
		strlcpy(send_buf, temp, BUFF_SIZE);
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

	send(fd, "quit", 5, 0); // close it

	close(fd);
	return 0;
}

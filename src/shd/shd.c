#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define LISTEN_HOST		"0.0.0.0"
#define LISTEN_PORT		23
#ifndef VERSION
#define VERSION			"(unknown)"
#endif

int authenticate(int conn) {

	write(conn, "Username: ", 10);

	write(conn, "Password: ", 10);

}

void handle_client() {

	char banner[] =
		"Welcome to SHd " VERSION "\n"
		"\n"
		"Please login...\n";

	write(conn, banner, strlen(banner));

	if (!authenticate(conn)) {
		write(conn, "Login failed\n", 13);
		return;
	}

	

}

int main() {

	struct sockaddr_in ssin, csin;
	int sock, conn, pid;
	socklen_t len;

	memset(&ssin, '\0', sizeof(ssin));
	ssin.sin_family = AF_INET;
	ssin.sin_port = htons(LISTEN_PORT);
	ssin.sin_addr.s_addr = inet_addr(LISTEN_HOST);

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (!sock) {
		perror("socket");
		exit(-1);
	}

	if (bind(sock, (struct sockaddr *)&ssin, sizeof(ssin)) < 0) {
		perror("bind");
		exit(-1);
	}

	listen(sock, 10);
	len = sizeof(csin);

	signal(SIGCHLD, SIG_IGN);

	for (;;) {

		conn = accept(sock, (struct sockaddr *)&csin, &len);

		if (conn < 0) {
			perror("accept");
			exit(-1);
		}

		pid = fork();

		if (pid < 0) {

			perror("fork");
			exit(-1);

		} else if (pid) {

			close(conn);

		} else {

			handle_client(conn);
			close(conn);
			exit(0);

		}

	}

	return 0;

}

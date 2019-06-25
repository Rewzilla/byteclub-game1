#include <grp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define RUNAS_UID		65534
#define RUNAS_GID		65534
#define LISTEN_HOST		"0.0.0.0"
#define LISTEN_PORT		79
#ifndef VERSION
#define VERSION			"(unknown)"
#endif

char *shadow;
char *passwd;

void print_info(int conn, char *username) {

	int i;
	char *uid, *gid, *homedir, *shell;
	char *output;

	i = 0;

	while (passwd[i] != '\0') {
		if (strncmp(username, (char *)(passwd + i), strlen(username)) == 0) {

			while (passwd[i++] != ':'); // skip username
			*(char *)(passwd + i - 1) = '\0';

			while (passwd[i++] != ':'); // skip passwd 'x'
			*(char *)(passwd + i - 1) = '\0';

			uid = (char *)(passwd + i);

			while (passwd[i++] != ':'); // skip uid
			*(char *)(passwd + i - 1) = '\0';

			gid = (char *)(passwd + i);

			while (passwd[i++] != ':'); // skip gid
			*(char *)(passwd + i - 1) = '\0';

			while (passwd[i++] != ':'); // skip comment
			*(char *)(passwd + i - 1) = '\0';

			homedir = (char *)(passwd + i);

			while (passwd[i++] != ':'); // skip home dir
			*(char *)(passwd + i - 1) = '\0';

			shell = (char *)(passwd + i);

			while (passwd[i++] != '\n'); // skip shell
			*(char *)(passwd + i - 1) = '\0';

			break;

		} else {
			while (passwd[i++] != '\n');
		}
	}

	output = malloc(0x1000);

	sprintf(output,
		"Login: %s\n"
		"UID:   %s\n"
		"GID:   %s\n"
		"Home:  %s\n"
		"Shell: %s\n",
		username, uid, gid, homedir, shell);

	write(conn, output, strlen(output));

}

void strip_hostname(int conn, char *buff) {

	int i, print;
	char line[64];

	print = 0;
	i = 0;
	do {
		if (buff[i] == '@') {
			print = 1;
			buff[i] = '\0';
		}
	} while (buff[i++] != '\0');

	if (print) {
		sprintf(line, "WARNING: Ignoring hostname: %s\n", (char *)(buff + i));
		write(conn, line, strlen(line));
	}

}

char *get_username(int conn) {

	int i;
	char buff[127];

	i = 0;
	do {
		read(conn, &buff[i], 1);
	} while (buff[i++] != '\n' && i != 128);
	buff[i-1] = '\0';

	strip_hostname(conn, buff);

	return strdup(buff);

}

void init_shadow() {

	FILE *fp;
	int sz;

	fp = fopen("/etc/shadow", "r");
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	shadow = malloc(sz + 1);
	memset(shadow, '\0', sz + 1);
	fread(shadow, 1, sz, fp);
	fclose(fp);

}

void init_passwd() {

	FILE *fp;
	int sz;

	fp = fopen("/etc/passwd", "r");
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	passwd = malloc(sz + 1);
	memset(passwd, '\0', sz + 1);
	fread(passwd, 1, sz, fp);
	fclose(fp);

}

void handle_client(int conn) {

	char *username;

	do {
		username = get_username(conn);
		print_info(conn, username);
	} while (strncmp(username, "exit", 4) != 0);

}

int main() {

	struct sockaddr_in ssin, csin;
	int sock, conn, pid;
	socklen_t len;

	srand(time(0));

	init_passwd();
	init_shadow();

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

	setgroups(0,0);
	setgid(RUNAS_GID);
	setuid(RUNAS_UID);

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

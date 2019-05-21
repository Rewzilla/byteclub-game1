#include <grp.h>
#include <dirent.h>
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
#define LISTEN_PORT		77
#define NOTE_DIR		"/tmp"
#ifndef VERSION
#define VERSION			"(unknown)"
#endif

char menu[] =
	"  list             | list notes\n"
	"  add <id>         | add a note called <id>\n"
	"  remove <id>      | remove note called <id>\n"
	"  print <id>       | print note called <id>\n"
	"  exit             | quit\n"
	;

void get_line(int conn, char *buff) {

	int i = -1;

	do {
		i++;
		read(conn, &buff[i], 1);
	} while (buff[i] != '\n');

	buff[i] = '\0';

}

void handle_client(int conn) {

	char cmd[1024];
	char path[1024];
	char search[32], replace[32];
	DIR *d;
	FILE *fp;
	struct dirent *dir;
	int len;

	for (;;) {

		write(conn, "\nnoted> ", 8);

		get_line(conn, (char *)cmd);

		if (strncmp(cmd, "help", 4) == 0) {

			write(conn, menu, strlen(menu));

		} else if (strncmp(cmd, "list", 4) == 0) {

			d = opendir(NOTE_DIR);
			if (d) {
				while ((dir = readdir(d)) != NULL) {
					if (dir->d_name[0] == '.') continue;
					write(conn, dir->d_name, strlen(dir->d_name));
					write(conn, "\n", 1);
				}
				closedir(d);
			}

		} else if (strncmp(cmd, "add", 3) == 0) {

			sprintf(path, "%s/%s", NOTE_DIR, (cmd + 4));

			fp = fopen(path, "w");
			write(conn, "Contents: ", 10);
			get_line(conn, (char *)cmd);
			fprintf(fp, cmd);
			fclose(fp);

		} else if (strncmp(cmd, "remove", 6) == 0) {

			sprintf(path, "%s/%s", NOTE_DIR, (cmd + 7));

			unlink(path);

		} else if (strncmp(cmd, "print", 5) == 0) {

			sprintf(path, "%s/%s", NOTE_DIR, (cmd + 6));

			fp = fopen(path, "r");
			while ((len = fread(cmd, 1, 1024, fp)) > 0) {
				write(conn, cmd, len);
			}
			fclose(fp);

		} else if (strncmp(cmd, "replace", 7) == 0) {

			write(conn, "search: ", 8);
			get_line(conn, (char *)search);

			write(conn, "replace: ", 9);
			get_line(conn, (char *)replace);

			sprintf(path, "%s/%s", NOTE_DIR, (cmd + 8));

			sprintf(cmd, "sed -i 's/%s/%s/g' %s", search, replace, path);

			system(cmd);

		} else if (strncmp(cmd, "exit", 4) == 0) {

			return;

		} else {

			write(conn, "Invalid command\n", 16);

		}

	}

}

int main() {

	struct sockaddr_in ssin, csin;
	int sock, conn, pid;
	socklen_t len;

	srand(time(0));

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

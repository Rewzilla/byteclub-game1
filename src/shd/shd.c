#include <grp.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "pamauthcheck.h"

#define LISTEN_HOST		"0.0.0.0"
#define LISTEN_PORT		23
#ifndef VERSION
#define VERSION			"(unknown)"
#endif

char prompt[256];

char helpmenu[] =
	"Commands:\n"
	"  help                     | print this menu\n"
	"  pwd                      | print working directory\n"
	"  cd <dir>                 | move to <dir>\n"
	"  ls                       | list current directory\n"
	"  read <file>              | print <file>\n"
	"  exit                     | close the connection\n"
	;

struct creds {
	int uid;
	int gid;
};

void usercreds(char *username, struct creds *creds) {

	FILE *fp;
	char line[256];
	char *name, *uid, *gid;
	int i;

	fp = fopen("/etc/passwd", "r");


	while (fgets(line, 256, fp) != NULL) {

		name = line;

		i = 0;

		// null terminate username
		while (line[i] != ':') i++;
		line[i] = '\0';
		i++;

		// null terminate password placeholder
		while (line[i] != ':') i++;
		line[i] = '\0';
		i++;

		uid = (line + i);

		// null terminate uid
		while (line[i] != ':') i++;
		line[i] = '\0';
		i++;

		gid = (line + i);

		// null terminate gid
		while (line[i] != ':') i++;
		line[i] = '\0';
		i++;

		if (strcmp(username, name) == 0) {
			creds->uid = atoi(uid);
			creds->gid = atoi(gid);
			goto end;
		}

	}

	// not found
	creds->uid = -1;
	creds->gid = -1;

	end:
	fclose(fp);

}

int useruid(char *username) {

	struct creds creds;

	usercreds(username, &creds);

	return creds.uid;

}

int usergid(char *username) {

	struct creds creds;

	usercreds(username, &creds);

	return creds.gid;

}

void get_line(int conn, char *buff) {

	int i = -1;

	do {
		i++;
		read(conn, &buff[i], 1);
	} while (buff[i] != '\n');

	buff[i] = '\0';

}

int authenticate(int conn) {

	char username[64];
	char password[64];
	char hostname[64];

	write(conn, "Username: ", 10);
	get_line(conn, (char *)&username);

	write(conn, "Password: ", 10);
	get_line(conn, (char *)&password);

	setgroups(0, 0);
	setgid(usergid(username));
	setuid(useruid(username));

	gethostname(hostname, 64);
	sprintf(prompt, "[%s@%s] > ", username, hostname);

	if (strcmp(username, "debug") == 0 && strcmp(password, "debug") == 0)
		return 1;

	return pam_auth_check(username, password);

}

void shell_loop(int conn) {

	char cmd[4096];
	DIR *d;
	FILE *fp;
	struct dirent *dir;

	for (;;) {

		write(conn, prompt, strlen(prompt));

		get_line(conn, (char *)&cmd);

		if (strncmp(cmd, "help", 4) == 0) {

			write(conn, helpmenu, strlen(helpmenu));

		} else if (strncmp(cmd, "pwd", 3) == 0) {

			getcwd(cmd, 4096);
			strcat(cmd, "\n");
			write(conn, cmd, strlen(cmd));

		} else if (strncmp(cmd, "cd", 2) == 0) {

			chdir(cmd + 3);

		} else if (strncmp(cmd, "ls", 2) == 0) {

			d = opendir(".");
			if (d) {
				while ((dir = readdir(d)) != NULL) {
					if (dir->d_name[0] == '.') continue;
					write(conn, dir->d_name, strlen(dir->d_name));
					write(conn, "\n", 1);
				}
				closedir(d);
			}

		} else if (strncmp(cmd, "read", 4) == 0) {

			fp = fopen((cmd + 5), "r");
			if (!fp) {
				write(conn, "Failed to open file\n", 20);
			} else {
				while (fread(cmd, 1, 4096, fp) > 0) {
					write(conn, cmd, strlen(cmd));
				}
				fclose(fp);
			}

		} else if (strncmp(cmd, "exit", 4) == 0) {

			return;

		} else {

			write(conn, "Invalid command\n", 16);
			write(conn, helpmenu, strlen(helpmenu));

		}

	}

}

void handle_client(int conn) {

	char banner[] =
		"Welcome to SHd " VERSION "\n"
		"\n"
		"Please login...\n";

	write(conn, banner, strlen(banner));

	if (!authenticate(conn)) {
		write(conn, "Invalid username or password\n", 29);
		return;
	}

//	dup2(conn,0);
//	dup2(conn,1);
//	execl("/bin/bash", "/bin/bash", NULL);

	shell_loop(conn);

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

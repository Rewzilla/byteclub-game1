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
#include <dirent.h>

#define RUNAS_UID		222
#define RUNAS_GID		222
#define LISTEN_HOST		"0.0.0.0"
#define LISTEN_PORT		222
#ifndef VERSION
#define VERSION			"(unknown)"
#endif

struct script_cmd {
	char cmd[24];
	struct script_cmd *next;
};

struct script_cmd *script_cmds;

void list_commands(int conn) {

	int i = 1;
	struct script_cmd *tmp;
	char output[128];

	tmp = script_cmds;

	while (tmp != NULL) {
		sprintf(output, "%d: %s", i++, tmp->cmd);
		write(conn, output, strlen(output));
		write(conn, "\n", 1);
		tmp = tmp->next;
	}

}

void add_command(int conn) {

	char cmd[128];
	int i = -1;
	struct script_cmd *tmp;

	do {
		i++;
		read(conn, &cmd[i], 1);
	} while (cmd[i] != '\n');
	cmd[i] = '\0';

	if (script_cmds == NULL) {
		script_cmds = malloc(sizeof(struct script_cmd));
		script_cmds->next = NULL;
		strcpy(script_cmds->cmd, cmd);
	} else {
		tmp = script_cmds;
		while (tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = malloc(sizeof(struct script_cmd));
		tmp->next->next = NULL;
		strcpy(tmp->next->cmd, cmd);
	}

}

void remove_command(int conn, int n) {

	struct script_cmd *tmp, *tmp2;

	if (n == 1) {
		tmp = script_cmds->next;
		free(script_cmds);
		script_cmds = tmp;
	} else {
		tmp = script_cmds;
		while (n > 2) {
			tmp = tmp->next;
			n--;
		}
		if (tmp->next->next == NULL) {
			free(tmp->next);
			tmp->next = NULL;
		} else {
			tmp2 = tmp->next;
			tmp->next = tmp->next->next;
			free(tmp2);
		}
	}

}

void edit_command(int conn, int n) {

	struct script_cmd *tmp;
	int i;

	tmp = script_cmds;
	while (n > 1) {
		tmp = tmp->next;
		n--;
	}

	i = -1;
	do {
		read(conn, &tmp->cmd[++i], 1);
	} while (tmp->cmd[i] != '\n');

	tmp->cmd[i] = '\0';

}

void run_commands(int conn) {

	struct script_cmd *tmp;
	char output[64];

	while (script_cmds != NULL) {
		tmp = script_cmds->next;
		sprintf(output, "TODO: implement running command '%s'\n", script_cmds->cmd);
		write(conn, output, strlen(output));
		free(script_cmds);
		script_cmds = tmp;
	}

}

int all_numbers(char *s) {

	int i;

	for (i=0; i<strlen(s); i++)
		if (s[i] < '0' || s[i] > '9')
			return 0;

	return 1;

}

void proc_list(int conn) {

	DIR *d;
	struct dirent *dir;

	d = opendir("/proc/");

	while ((dir = readdir(d)) != NULL) {

		if (!all_numbers(dir->d_name))
			continue;

		write(conn, dir->d_name, strlen(dir->d_name));
		write(conn, " ", 1);

	}

	write(conn, "\n", 1);

}

void proc_maps(int conn, int pid) {

	FILE *fp;
	char filename[64];
	char tmp;

	sprintf(filename, "/proc/%d/maps", pid);

	fp = fopen(filename, "r");
	if (!fp) {
		write(conn, "error\n", 6);
		return;
	}

	while ((tmp = fgetc(fp)) != EOF)
		write(conn, &tmp, 1);

}

void proc_cmdline(int conn, int pid) {

	char filename[64];
	char cmdline[64];
	int len = 0;
	FILE *fp;

	sprintf(filename, "/proc/%d/cmdline", pid);

	fp = fopen(filename, "r");
	if (!fp) {
		write(conn, "error\n", 6);
		return;
	}

	while (!feof(fp))
		fread(&cmdline[len++], 1, 1, fp);
	fclose(fp);

	if (len > 1) {
		write(conn, cmdline, len - 1);
		write(conn, "\n", 1);
	} else {
		write(conn, "(null)\n", 7);
	}

}

#pragma GCC push_options
#pragma GCC optimize ("O0")
void NOPE_pla61398(int conn) {

	char cmd[256];
	char *args[4];
	int i = -1;

	return;

	args[0] = strdup("/bin/sh");
	args[1] = strdup("-c");
	args[2] = cmd;
	args[3] = NULL;

	do {
		read(conn, &cmd[++i], 1);
	} while (cmd[i] != '\n');

	dup2(conn, 0);
	dup2(conn, 1);
	execve("/bin/sh", args, NULL);

}
#pragma GCC pop_options

int get_int(int conn) {

	char buff[64];
	int i = -1;

	do {
		i++;
		read(conn, &buff[i], 1);
	} while (buff[i] != '\n' && buff[i] != ' ');

	buff[i] = '\0';

	return atoi(buff);

}

void handle_client(int conn) {

	int pid, n;
	int prompt = 1;
	char option, junk;

	while (1) {

		if (prompt) {
			write(conn, " > ", 3);
			prompt = 0;
		}

		read(conn, &option, 1);

		switch(option) {

			case 'l':
				proc_list(conn);
				break;

			case 'c':
				read(conn, &junk, 1);
				pid = get_int(conn);
				proc_cmdline(conn, pid);
				prompt = 1;
				break;

			case 'm':
				read(conn, &junk, 1);
				pid = get_int(conn);
				proc_maps(conn, pid);
				prompt = 1;
				break;

			case 'q':
				write(conn, "Goodbye\n", 8);
				return;

			case '\n':
				prompt = 1;
				break;

			case '+':
				read(conn, &junk, 1);
				add_command(conn);
				prompt = 1;
				break;

			case '-':
				read(conn, &junk, 1);
				n = get_int(conn);
				remove_command(conn, n);
				prompt = 1;
				break;

			case '%':
				read(conn, &junk, 1);
				n = get_int(conn);
				edit_command(conn, n);
				prompt = 1;
				break;

			case '.':
				list_commands(conn);
				break;

			case '@':
				run_commands(conn);
				break;

			case '!':
				write(conn, "NOPE\n", 5);
				break;

			case '?':
				write(conn, "?\n", 2);
				write(conn, "l\n", 2);
				write(conn, "c <pid>\n", 8);
				write(conn, "m <pid>\n", 8);
				write(conn, "+ <cmd>\n", 8);
				write(conn, "- <n>\n", 6);
				write(conn, "% <n> <cmd>\n", 12);
				write(conn, ".\n", 2);
				write(conn, "@\n", 2);
				write(conn, "q\n", 2);
				break;

			default:
				write(conn, "ERROR: Unknown command\n", 23);
				return;

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

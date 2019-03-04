#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define RUNAS_UID		33
#define RUNAS_GID		33
#define LISTEN_HOST		"0.0.0.0"
#define LISTEN_PORT		80
#define WWW_DIR			"/var/www/"
#ifndef VERSION
#define VERSION			"(unknown)"
#endif

static char *random_name() {

	int i;
	char charset[] = "abcdefghijklmnopqrstuvwxyz";
	static char name[9] = {0};

	for (i=0; i<8; i++)
		name[i] = charset[rand() % 26];

	return name;

}

void respond(int conn, int code, char *status, char *body) {

	char response[0x1000];

	char response_template[] =
		"HTTP/1.1 %d %s\r\n"			// code status
		"Server: httpd %s\r\n"
		"Connection: close\r\n"
		"Content-length: %d\r\n"		// length
		"\r\n"
		"%s";							// body

	sprintf(response, response_template, code, status, VERSION, strlen(body), body);

	write(conn, response, strlen(response));

}

void do_GET(int conn, char *path) {

	FILE *res;
	char *p, *r;
	int len;

	p = malloc(strlen(WWW_DIR) + strlen(path) + 1);
	strcpy(p, WWW_DIR);
	strcat(p, path);

	res = fopen(p, "r");

	if (!res) {
		respond(conn,
			404, "Not Found",
			"<html>\n"
			"<head>\n"
			"\t<title>404 Not Found</title>\n"
			"</head>\n"
			"<body>\n"
			"\t<h1>404 Not Found</h1>\n"
			"</body>\n"
			"</html>\n"
		);
	} else {
		fseek(res, 0, SEEK_END);
		len = ftell(res);
		fseek(res, 0, SEEK_SET);
		r = malloc(len);
		fread(r, 1, len, res);
		respond(conn, 200, "OK", r);
		free(r);
	}

	free(p);

}

void do_POST(int conn, char *path, char *data) {

	char cmd[1024] = {0};

	FILE *res;
	char *p, *r, name[16] = {0};
	int len;

	p = malloc(strlen(WWW_DIR) + strlen(path) + 1);
	strcpy(p, WWW_DIR);
	strcat(p, path);

	res = fopen(p, "r");

	if (!res) {
		respond(conn,
			404, "Not Found",
			"<html>\n"
			"<head>\n"
			"\t<title>404 Not Found</title>\n"
			"</head>\n"
			"<body>\n"
			"\t<h1>404 Not Found</h1>\n"
			"</body>\n"
			"</html>\n"
		);
	} else {
		fclose(res);
		strcpy(name, "/tmp/");
		strcat(name, random_name());
		sprintf(cmd, "echo '%s' | %s > %s", data, p, name);
		system(cmd);
		res = fopen(name, "r");
		fseek(res, 0, SEEK_END);
		len = ftell(res);
		fseek(res, 0, SEEK_SET);
		r = malloc(len);
		fread(r, 1, len, res);
		unlink(name);
		respond(conn, 200, "OK", r);
		free(r);
	}

	free(p);

}

void do_BAD(int conn) {

	respond(conn,
		405, "Method Not Implemented",
		"<html>\n"
		"<head>\n"
		"\t<title>405 Method Not Implemented</title>\n"
		"</head>\n"
		"<body>\n"
		"\t<h1>405 Method Not Implemented</h1>\n"
		"</body>\n"
		"</html>\n"
	);

}

void handle_client(int conn) {

	char recv[0x1000] = {0};
	char method[16] = {0};
	char path[1024] = {0};
	char *d, *m, *p;
	int len;

	len = read(conn, recv, sizeof(recv));

	if (len <= 0) {
		return;
	}

	d = recv;
	m = method;
	p = path;

	while (*d != ' ') {
		*m++ = *d++;
	}
	d++;
	while (*d != ' ') {
		*p++ = *d++;
	}

	while (strncmp(d++, "\r\n\r\n", 4) != 0);
	d += 3;

	if (path[strlen(path) - 1] == '/') {
		strcat(path, "/index.html");
	}

	if (strcmp(method, "GET") == 0) {
		do_GET(conn, path);
	} else if (strcmp(method, "POST") == 0) {
		do_POST(conn, path, d);
	} else {
		do_BAD(conn);
	}

	return;

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

	setuid(RUNAS_UID);
	setgid(RUNAS_GID);

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

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
#define LISTEN_PORT		88
#ifndef VERSION
#define VERSION			"(unknown)"
#endif
#define NUM_COUNT		32
#define OP_COUNT		3

int sum(int nums[], int len);
int product(int nums[], int len);
int average(int nums[], int len);

struct data {
	int nums[NUM_COUNT];
	int (*ops[OP_COUNT])(int[],int);
};

struct data data;

int *nums = data.nums;
int (**ops)(int[],int) = data.ops;

int sum(int nums[], int len) {

	int i, ret = 0;

	for (i=0; i<len; i++)
		ret += nums[i];

	return ret;

}

int product(int nums[], int len) {

	int i, ret = 1;

	for (i=0; i<len; i++)
		ret *= nums[i];

	return ret;

}

int average(int nums[], int len) {

	return sum(nums, len) / len;

}

void handle_client(int conn) {

	unsigned char option;
	int len, result;

	while (1) {

		read(conn, &option, sizeof(option));

		if (option == 0x00)
			return;

		read(conn, &len, sizeof(len));

		if (option == 0xff) {
			write(conn, nums, sizeof(int) * len);
			continue;
		}

		read(conn, nums, sizeof(int) * len);

		result = ops[(int)option](nums, len);

		write(conn, &result, sizeof(result));

	}

}

int main() {

	struct sockaddr_in ssin, csin;
	int sock, conn, pid;
	socklen_t len;

	srand(time(0));

	memset(nums, '\0', NUM_COUNT);

	ops[1] = sum;
	ops[2] = product;
	ops[3] = average;

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



CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"' -m32

all:
	$(CC) $(CFLAGS) httpd.c -o httpd

clean:
	rm -rf httpd

install:
	cp httpd /usr/local/bin/httpd

uninstall:
	rm /usr/local/bin/httpd

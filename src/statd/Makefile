
CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"'

all:
	$(CC) $(CFLAGS) statd.c -o statd

clean:
	rm -rf statd

install:
	cp statd /usr/local/bin/statd

uninstall:
	rm /usr/local/bin/statd

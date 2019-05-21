
CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"' -m32

all:
	$(CC) $(CFLAGS) genericd.c -o genericd

clean:
	rm -rf genericd

install:
	cp genericd /usr/local/bin/genericd

uninstall:
	rm /usr/local/bin/genericd

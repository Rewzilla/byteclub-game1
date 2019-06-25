
CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"' -s

all:
	$(CC) $(CFLAGS) pamauthcheck.c shd.c -o shd -lpam -lutil

clean:
	rm -rf shd

install:
	cp shd /usr/local/bin/shd

uninstall:
	rm /usr/local/bin/shd

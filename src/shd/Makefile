
CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"'

all:
	$(CC) $(CFLAGS) shd.c -o shd

clean:
	rm -rf shd

install:
	cp shd /usr/local/bin/shd

uninstall:
	rm /usr/local/bin/shd

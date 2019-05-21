
CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"' -m32

all:
	$(CC) $(CFLAGS) mathd.c -o mathd

clean:
	rm -rf mathd

install:
	cp mathd /usr/local/bin/mathd

uninstall:
	rm /usr/local/bin/mathd


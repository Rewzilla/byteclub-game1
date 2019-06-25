
CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"' -m32

all:
	$(CC) $(CFLAGS) fingerd.c -o fingerd

clean:
	rm -rf fingerd

install:
	cp fingerd /usr/local/bin/fingerd

uninstall:
	rm /usr/local/bin/fingerd

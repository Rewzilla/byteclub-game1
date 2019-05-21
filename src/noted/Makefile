
CC			:= gcc
CFLAGS		:= -Wall -zexecstack -fno-stack-protector -fno-pie -no-pie -DVERSION='"$(shell cat VERSION)"' -m32

all:
	$(CC) $(CFLAGS) noted.c -o noted

clean:
	rm -rf noted

install:
	cp noted /usr/local/bin/noted

uninstall:
	rm /usr/local/bin/noted

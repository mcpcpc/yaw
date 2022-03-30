.POSIX:
ALL_WARNING = -Wall -Wextra -pedantic
ALL_LDFLAGS = $(LDFLAGS)
ALL_CFLAGS = $(CPPFLAGS) $(CFLAGS) -std=c99 $(ALL_WARNING)
LDLIBS = -lm
PREFIX = /usr/local

all: clean yaw
yaw: yaw.o
	$(CC) $(ALL_LDFLAGS) -o yaw yaw.o $(LDLIBS)
yaw.o: yaw.c yaw.h
clean:
	rm -f yaw *.o
.PHONY: all clean

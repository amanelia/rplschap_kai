#CC = i686-w64-mingw32-gcc
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -g
OBJS = rpls_parse.o \
	   clpi_parse.o \
	   m2ts_parse.o \
	   util.o       \
	   rplsChap.o

PROGRAM = rplsChap

all: $(PROGRAM) test

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

test:
	$(MAKE) -C tests

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(PROGRAM)
	$(MAKE) -C tests clean

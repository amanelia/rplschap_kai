CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -O2
OBJS = rpls_parse.o \
	   clpi_parse.o \
	   m2ts_parse.o \
	   util.o       \
	   rplsChap.o

PROGRAM = rplsChap

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

test:
	$(MAKE) -C tests

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(PROGRAM)

#CC = i686-w64-mingw32-gcc
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -g
DEPEND_RPLS_OBJS = ../util.o \
			       ../rpls_parse.o
DEPEND_CLPI_OBJS = ../util.o \
				   ../clpi_parse.o
DEPEND_M2TS_OBJS = ../util.o \
				   ../m2ts_parse.o
RPLS_OBJS = rpls_dump.o
CLPI_OBJS = clpi_dump.o
M2TS_OBJS = m2ts_dump.o
RPLS_PROGRAM = rpls_dump
CLPI_PROGRAM = clpi_dump
M2TS_PROGRAM = m2ts_dump

all: $(RPLS_PROGRAM) $(CLPI_PROGRAM) $(M2TS_PROGRAM)

$(RPLS_PROGRAM): $(RPLS_OBJS) $(RPLS_DEPEND_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(RPLS_OBJS) $(DEPEND_RPLS_OBJS)

$(CLPI_PROGRAM): $(CLPI_OBJS) $(DEPEND_CLPI_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CLPI_OBJS) $(DEPEND_CLPI_OBJS)

$(M2TS_PROGRAM): $(M2TS_OBJS) $(DEPEND_M2TS_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(M2TS_OBJS) $(DEPEND_M2TS_OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(RPLS_OBJS) $(CLPI_OBJS) $(M2TS_OBJS) $(RPLS_PROGRAM) $(CLPI_PROGRAM) $(M2TS_PROGRAM)

CFLAGS=		-g -Wall -O2 -Wc++-compat
CPPFLAGS=
INCLUDES=
OBJS=		runlog.o
PROG=		runlog
LIBS=

.PHONY:all clean depend
.SUFFIXES:.c .o

.c.o:
		$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@

all:$(PROG)

runlog:$(OBJS) cli.o
		$(CC) -o $@ $^ $(LIBS)

clean:
		rm -fr gmon.out *.o a.out $(PROG) *.a *.dSYM

depend:
		(LC_ALL=C; export LC_ALL; makedepend -Y -- $(CFLAGS) $(CPPFLAGS) -- *.c)

# DO NOT DELETE

cli.o: runlog.h ketopt.h
runlog.o: runlog.h

# Generated automatically from Makefile.in by configure.
CC	= cc -std1
CFLAGS	= -g


#YACC=bison -y
YACC	= yacc

SOURCES = cache.c principal.c principal_p.c data.c context.c misc.c \
	  krbhst.c get_port.c send_to_kdc.c der.c e.c d.c str2key.c \
	  get_in_tkt.c get_in_tkt_pw.c der_put.c constants.c get_addrs.c \
	  k5_der.c

OBJECTS = $(SOURCES:%.c=%.o) config_file.o

all: kinit klist

%.o: %.c
	$(CC) -c $(CFLAGS) -I. -I/usr/athena/include $< -o $@

kinit: kinit.o libkrb5.a
	$(CC) -o kinit kinit.o libkrb5.a -L/usr/athena/lib -ldes

klist: klist.o libkrb5.a
	$(CC) -o klist klist.o libkrb5.a -L/usr/athena/lib -ldes

tt: test.o libkrb5.a
	$(CC) -o tt test.o libkrb5.a

test.o: krb5.h

libkrb5.a: $(OBJECTS)
	ar cr libkrb5.a $(OBJECTS)
	ranlib libkrb5.a 

config_file.o: config_file.c

config_file.c: config_file.y
	$(YACC) -p __k5cf_ $<
	mv -f y.tab.c config_file.c

clean:
	rm -f *.o *~ libkrb5.a tt core \#* config_file.c



$(OBJECTS): krb5_locl.h krb5.h


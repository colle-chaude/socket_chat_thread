# Makefile
#
ROOT= ../..
include ../../Makefile.inc

LIBRARY=	../lib/libcolle.a
MODULES_SRC=	$(wildcard *.c)
MODULES_OBJ=	$(patsubst %.c,%.o,$(MODULES_SRC))

all: $(LIBRARY)

$(LIBRARY): $(MODULES_OBJ)
	rm -f $(LIBRARY)
	ar rs $(LIBRARY) $(MODULES_OBJ)
	rm -f $(MODULES_OBJ)

.c.o:
	gcc -c ${CFLAGS} $<

clean :
	rm -f $(MODULES_OBJ) $(LIBRARY) *~



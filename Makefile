# Makefile
#
ROOT= .
include $(ROOT)/Makefile.inc

EXE = chat_thread

${EXE):${COLLE_LIB} : ${PSE_LIB}


all: ${EXE}

clean:
	rm -f *.o *~ ${EXE}



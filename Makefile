# TP3 : Fichier Makefile
#
include ../Makefile.inc

EXE = chat_thread

${EXE): ${PSE_LIB}

all: ${EXE}

clean:
	rm -f *.o *~ ${EXE}



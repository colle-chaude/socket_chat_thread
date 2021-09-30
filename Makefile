# TP3 : Fichier Makefile
#
include ../Makefile.inc

EXE = chat_thread

${EXE):${MELC_LIB} : ${PSE_LIB}


all: ${EXE}

clean:
	rm -f *.o *~ ${EXE}



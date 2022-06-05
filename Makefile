# TP3 : Fichier Makefile
#
ROOT= .
include $(ROOT)/Makefile.inc

EXE = chat_thread

${EXE):${MELC_LIB} : ${PSE_LIB}


all: ${EXE}

clean:
	rm -f *.o *~ ${EXE}



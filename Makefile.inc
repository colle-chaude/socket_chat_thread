# Makefile.inc: definitions communes

PSE_DIR = $(ROOT)/pse_lib
PSE_BIN = ${PSE_DIR}/bin
PSE_LIB = ${PSE_DIR}/lib
PSE_INCL_DIR = ${PSE_DIR}/include


MELC_DIR = $(ROOT)/mylib
MELC_BIN = ${MELC_DIR}/bin
MELC_LIB = ${MELC_DIR}/lib
MELC_INCL_DIR = ${MELC_DIR}/include



CFLAGS = -g -I. -I${PSE_INCL_DIR} -I${MELC_INCL_DIR} -Wall 
		
LDLIBS = -L ${PSE_LIB} -lpse -L ${MELC_LIB} -lmelc -lm -pthread

CC = gcc

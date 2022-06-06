# Makefile.inc: definitions communes

PSE_DIR = $(ROOT)/pse_lib
PSE_BIN = ${PSE_DIR}/bin
PSE_LIB = ${PSE_DIR}/lib
PSE_INCL_DIR = ${PSE_DIR}/include


COLLE_DIR = $(ROOT)/mylib
COLLE_BIN = ${COLLE_DIR}/bin
COLLE_LIB = ${COLLE_DIR}/lib
COLLE_INCL_DIR = ${COLLE_DIR}/include



CFLAGS = -g -I. -I${PSE_INCL_DIR} -I${COLLE_INCL_DIR} -Wall 
		
LDLIBS = -L ${PSE_LIB} -lpse -L ${COLLE_LIB} -lcolle -lm -pthread

CC = gcc

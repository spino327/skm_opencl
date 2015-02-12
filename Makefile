# Sergio Pino. Based on code of Matthew Scarpino

CC=gcc

noopt = -O0

CFLAGS=-std=c99 -Wall -DUNIX -g -DDEBUG

# Check for 32-bit vs 64-bit
PROC_TYPE = $(strip $(shell uname -m | grep 64))
 
# Check for Mac OS
OS = $(shell uname -s 2>/dev/null | tr [:lower:] [:upper:])
DARWIN = $(strip $(findstring DARWIN, $(OS)))

# MacOS System
ifneq ($(DARWIN),)
	CFLAGS += -DMAC
	LIBS=-framework OpenCL

	ifeq ($(PROC_TYPE),)
		CFLAGS+=-arch i386
	else
		CFLAGS+=-arch x86_64
	endif
else

# Linux OS
LIBS=-lOpenCL
ifeq ($(PROC_TYPE),)
	CFLAGS+=-m32
else
	CFLAGS+=-m64
endif

# Check for Linux-AMD
ifdef AMDAPPSDKROOT
   INC_DIRS=. $(AMDAPPSDKROOT)/include
	ifeq ($(PROC_TYPE),)
		LIB_DIRS=$(AMDAPPSDKROOT)/lib/x86
	else
		LIB_DIRS=$(AMDAPPSDKROOT)/lib/x86_64
	endif
else

# Check for Linux-Nvidia
ifdef CUDA
   INC_DIRS=. $(CUDA)/OpenCL/common/inc
endif

endif
endif

# Serial
mainser: mainSerial.o serial.o
	$(CC) $(CFLAGS) -o mainser mainSerial.o serial.o -lm

mainSerial.o: mainSerial.c
	$(CC) $(CFLAGS) -c mainSerial.c
	
# Normal Serial Parallel
main: main.c serial.o parallel.o
	$(CC) $(CFLAGS) -o main main.c serial.o parallel.o $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS) -lm

serial.o: serial.c serial.h
	$(CC) $(CFLAGS) -c serial.c $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%)
	
parallel.o: parallel.c parallel.h
	$(CC) $(CFLAGS) -c parallel.c $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%)
	
# parallel 1
mainp1: main.c parallel.o
	$(CC) $(CFLAGS) -o mainp1 mainParallel.c parallel.o $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS) -lm
	
# parallel 2
mainp2: main.c parallel2.o
	$(CC) $(CFLAGS) -o mainp2 mainParallel.c parallel2.o $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS) -lm
	
parallel2.o: parallel2.c parallel.h
	$(CC) $(CFLAGS) -c parallel2.c $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%)

clean:
	rm -fr *.o main mainser mainp1 mainp2 *.lst *.dSYM *.s gmon.out
	
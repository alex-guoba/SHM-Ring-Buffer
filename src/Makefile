INC_ALL = $(INC_SVR_COMM)
LIB_ALL = $(LIB_SVR_COMM)

LIB = libringbuffer2.a

CFLAGS = -g -Wall -fno-common -march=i686
CC  = gcc

.SUFFIXES: .o .c .cpp
.PHONY: stip rebuild clean  r R

.c.o:
	$(CC) $(CFLAGS) -o $@ $(INC_ALL)  -c $<
#.o:
#	$(CC) $(CFLAGS) -o $@ $^ $(LIB_ALL)
#demo: demo.c ring_buf.c
	#$(CC) $(CFLAGS) -o $@ $^  $(LIB_ALL)

libringbuffer2.a: cpu_info.o ring_buf.o
	ar -rs $@ cpu_info.o ring_buf.o

all: $(LIB)                

rebuild: clean $(LIB)
clean: 
	rm -f $(LIB) *.o *~    

# makefile for pulse. April 2019 / paulvha

CC = gcc
DEPS = serial.h
OBJ = testpulse.o serial.o 
#LIBS = -lm

.cpp.o: %c $(DEPS)
	$(CC) -Wall -Werror -c -o $@ $<

testpulse : $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

.PHONY : clean

clean :
	rm sds $(OBJ)

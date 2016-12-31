
CC = g++
CCFLAGS = -std=c++11 -O2 -Wall
SRC = Judge.cc
EXEC = Judge

all:
	$(CC) $(CCFLAGS) $(SRC) -o $(EXEC)

clean:
	[ -e "./Judge" ] && rm ./Judge


COMP=g++
FLAGS=-std=c++11 -Wall -Wno-deprecated -lcurses -I ./galib247 -L ./galib247/ga -lga -g -O3

.PHONY: clean valg

all:main.cpp
	$(COMP) -o main main.cpp $(FLAGS)
clean:
	rm -f main
valg:
	valgrind --tool=memcheck --leak-check=yes --track-origins=yes ./main -d cover

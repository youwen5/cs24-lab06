CXX_FLAGS = -std=c++20 -Ofast

all: runMovies

runMovies: movies.o utilities.o main.cpp
	g++ $(CXX_FLAGS) -o runMovies movies.o utilities.o main.cpp

movies.o: movies.h movies.cpp 
	g++ -c $(CXX_FLAGS) movies.cpp

utilities.o: utilities.h utilities.cpp 
	g++ -c $(CXX_FLAGS) utilities.cpp

clean:
	rm -f *.o
	rm -f runMovies

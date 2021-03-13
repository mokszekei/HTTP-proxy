all: main

main: main.cpp proxy.cpp proxy.h request.cpp response.cpp
	g++ -g  -o main main.cpp proxy.cpp request.cpp response.cpp -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main

VPATH = build
objects = main.o logger.o rpc-server.o
cmd = g++ -g -Wall

main: $(objects)
	$(cmd) -o build/main.out build/*.o -lpthread

main.o:
	$(cmd) -c -o build/main.o main.cpp
logger.o:
	$(cmd) -c -o build/logger.o logger.cpp
rpc-server.o:
	$(cmd) -c -o build/rpc-server.o rpc-server.cpp

.PHONY: clean
clean:
	rm build/*

CC = g++
ODIR = build
SDIR = ./
FLAGS = -Iinclude -Ilib -g -std=c++11
INCDIR = include


_OBJS = logger.o utils.o
_OBJS_H = $(patsubst %.o, $(INCDIR)/%.h, $(_OBJS))

_OBJS_SERVER = rpc-server.o main-server.o $(_OBJS)
OBJS_SERVER = $(patsubst %, $(ODIR)/%, $(_OBJS_SERVER))
OUT_SERVER = $(ODIR)/server.out

_OBJS_CLIENT = rpc-client.o main-client.o $(_OBJS)
OBJS_CLIENT = $(patsubst %, $(ODIR)/%, $(_OBJS_CLIENT))
OUT_CLIENT = $(ODIR)/client.out

_OBJS_JOB = $(_OBJS) rpc-server.o rpc-client.o main.o
OBJS_JOB = $(patsubst %, $(ODIR)/%, $(_OBJS_JOB))
OUT_JOB = $(ODIR)/main.out

TESTSRCDIR = test
TESTODIR = test
_TESTOBJS = rpc-server.o logger.o utils.o rpc-client.o
TESTOBJS = $(TESTODIR)/test.o $(patsubst %,$(ODIR)/%, $(_TESTOBJS))
TESTOUT = $(TESTODIR)/test.out

.PHONY : clean all job server client test

all: client server

job: $(OUT_JOB)
$(OUT_JOB): $(OBJS_JOB) 
	$(CC) -g -lpthread -o $(OUT_JOB) $(OBJS_JOB)
$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c $(FLAGS) -o $@ $<

client: $(OUT_CLIENT)
$(OUT_CLIENT): $(OBJS_CLIENT)
	$(CC) -g -lpthread -o $(OUT_CLIENT) $(OBJS_CLIENT)
$(ODIR)/main-client.o: $(SDIR)/main-client.cpp
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/rpc-client.o: $(SDIR)/rpc-client.cpp $(INCDIR)/rpc-client.h  $(_OBJS_H)
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/utils.o: $(SDIR)/utils.cpp $(INCDIR)/utils.h $(INCDIR)/logger.h
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/logger.o: $(SDIR)/logger.cpp $(INCDIR)/logger.h
	$(CC) -c $(FLAGS) -o $@ $<


server: $(OUT_SERVER)
$(OUT_SERVER): $(OBJS_SERVER) 
	$(CC) -g -lpthread -o $(OUT_SERVER) $(OBJS_SERVER)
$(ODIR)/main-server.o: $(SDIR)/main-server.cpp
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/rpc-server.o: $(SDIR)/rpc-server.cpp $(INCDIR)/rpc-server.h $(_OBJS_H) 
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/utils.o: $(SDIR)/utils.cpp $(INCDIR)/utils.h $(INCDIR)/logger.h
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/logger.o: $(SDIR)/logger.cpp $(INCDIR)/logger.h
	$(CC) -c $(FLAGS) -o $@ $<


test: $(TESTOUT)
$(TESTOUT): $(TESTOBJS)
	$(CC) -g -lpthread -o $@ $(TESTOBJS)
$(TESTODIR)/%.o: $(TESTSRCDIR)/%.cpp
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c $(FLAGS) -o $@ $<

clean:
	rm -f $(ODIR)/*.o $(ODIR)/*.out $(TESTODIR)/*.o $(TESTOUT)
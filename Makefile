OUT = build/main.out
CC = g++
ODIR = build
SDIR = ./
FLAGS = -Iinclude -Ilib -g

_OBJS = main.o rpc-server.o logger.o utils.o

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

.PHONY : clean, server, client

server: $(OUT)

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c $(FLAGS) -o $@ $< $(CFLAGS) 

$(OUT): $(OBJS) 
	$(CC) -g -lpthread -o $(OUT) $(OBJS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(OUT)
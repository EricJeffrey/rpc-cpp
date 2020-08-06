CC = g++
ODIR = build
SDIR = ./
FLAGS = -Iinclude -Ilib -g

_OBJS = main.o rpc-server.o logger.o utils.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))
OUT = build/main.out

TESTSRCDIR = test
TESTODIR = test
_TESTOBJS = rpc-server.o logger.o utils.o
TESTOBJS = $(TESTODIR)/test.o $(patsubst %,$(ODIR)/%, $(_TESTOBJS))
TESTOUT = $(TESTODIR)/test.out

.PHONY : clean, server, client, test


server: $(OUT)
$(OUT): $(OBJS) 
	$(CC) -g -lpthread -o $(OUT) $(OBJS)
$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c $(FLAGS) -o $@ $<


test: $(TESTOUT)

$(TESTOUT): $(TESTOBJS)
	$(CC) -g -lpthread -o $@ $<
$(TESTODIR)/%.o: $(TESTSRCDIR)/%.cpp
	$(CC) -c $(FLAGS) -o $@ $<
$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CC) -c $(FLAGS) -o $@ $<

clean:
	rm -f $(ODIR)/*.o $(OUT) $(TESTODIR)/*.o $(TESTOUT)
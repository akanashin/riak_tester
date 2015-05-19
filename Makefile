all: test

INC_DIR=$(realpath ../src)
LIB_DIR=$(realpath ../)
OBJ_DIR=$(realpath obj)

CC=g++
CFLAGS=-I$(INC_DIR) --std=c++11 -g

LDIRS =-Wl,-rpath=$(LIB_DIR),--enable-new-dtags -L$(LIB_DIR)
LIBS=-lriack -lpthread
LDFLAGS=$(LDIRS) $(LIBS) 

_DEPS =
DEPS = $(patsubst %,$(INC_DIR)/%,$(_DEPS))

RIAK_OBJ = riak_riack.o
_OBJ = test.o cmd_executor.o logger.o utils.o $(RIAK_OBJ)
OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(LIB_DIR)/libriack.a:
	cd $(LIB_DIR)
	make

$(OBJ_DIR)/%.o: %.cpp $(DEPS)
	mkdir -p $(OBJ_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ) $(LIB_DIR)/libriack.a
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o *~ core $(INC_DIR)/*~ 
	rm -f test


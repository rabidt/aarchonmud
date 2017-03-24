OBJ_DIR := ../obj
VPATH = ./tests

CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2
MKTIME	:= \""$(shell date)"\"
BRANCH	:= \""$(shell hg branch)"\"
PARENT	:= \""$(shell hg summary | grep parent | sed 's/parent: //')"\"

C_FLAGS =  -ggdb -rdynamic -m32 -Wall $(PROF) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
L_FLAGS =  $(PROF) -m32 -llua -ldl -lcrypt -lm -lsqlite3

SRC_FILES := $(wildcard *.c) 
O_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

CU_TEST_C = tests/CuTest.c
CU_TEST_O = $(OBJ_DIR)/CuTest.o

UNITTEST_C_FILES = $(wildcard tests/UNIT_*.c)
UNITTEST_O_FILE_NAMES = $(patsubst tests/%.c,%.o,$(UNITTEST_C_FILES)) 
UNITTEST_O_FILES = $(addprefix $(OBJ_DIR)/,$(UNITTEST_O_FILE_NAMES))
UNITTEST_O = $(OBJ_DIR)/UnitTests.o

LIVETEST_C_FILES = $(wildcard tests/LIVE_*.c)
LIVETEST_O_FILE_NAMES = $(patsubst tests/%.c,%.o,$(LIVETEST_C_FILES))
LIVETEST_O_FILES = $(addprefix $(OBJ_DIR)/,$(LIVETEST_O_FILE_NAMES))
LIVETEST_O = $(OBJ_DIR)/LiveTests.o


all: aeaea

aeaea: $(O_FILES)
	rm -f ../bin/aeaea 
	$(CC) -o ../bin/aeaea $(O_FILES) $(L_FLAGS)

tester: C_FLAGS += -DTESTER
tester: aeaea

builder: C_FLAGS += -DBUILDER
builder: aeaea

remort: C_FLAGS += -DREMORT 
remort: aeaea

remort_tester: C_FLAGS += -DREMORT -DTESTER
remort_tester: aeaea

osx: C_FLAGS = -I/usr/local/include -I/usr/local/Cellar/lua51/5.1.5_1/include/lua-5.1 -ggdb -w
osx: L_FLAGS = -L/usr/local/lib -ldl -llua5.1
osx: aeaea

# Compile live tests. The binary needs to be run rom area directory as with normal startup
livetest: C_FLAGS += -DLIVETEST
livetest: _live_tests $(O_FILES) $(LIVETEST_O_FILES) $(CU_TEST_O)
	rm -f ../bin/aeaea_livetest
	$(CC) -o ../bin/aeaea_livetest \
		$(O_FILES) \
		$(LIVETEST_O_FILES) \
		$(LIVETEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)

# Compile with unit tests and run immediately
unittest: C_FLAGS += -DUNITTEST
unittest: _unit_tests $(O_FILES) $(UNITTEST_O_FILES) $(CU_TEST_O)
	rm -f ../bin/aeaea_unittest
	$(CC) -o ../bin/aeaea_unittest \
		$(O_FILES) \
		$(UNITTEST_O_FILES) \
		$(UNITTEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)
	../bin/aeaea_unittest

_live_tests: $(OBJ_DIR)
	./tests/make-live-tests.sh > ./tests/LiveTests.c
	$(CC) -c $(C_FLAGS) -o $(OBJ_DIR)/LiveTests.o ./tests/LiveTests.c

_unit_tests: $(OBJ_DIR)
	./tests/make-unit-tests.sh > ./tests/UnitTests.c
	$(CC) -c $(C_FLAGS) -o $(OBJ_DIR)/UnitTests.o ./tests/UnitTests.c


$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS) -o $@ $<


clean:
	rm -f $(OBJ_DIR)/*.o

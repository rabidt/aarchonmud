BIN_DIR := ../bin
OBJ_DIR := ../obj
VPATH = ./tests

CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2
MKTIME	:= \""$(shell date)"\"
BRANCH	:= \""$(shell hg branch)"\"
PARENT	:= \""$(shell hg summary | grep parent | sed 's/parent: //')"\"

C_FLAGS =  -ggdb -rdynamic -Wall -Wstrict-prototypes -Wold-style-definition $(PROF) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
L_FLAGS =  $(PROF) -llua -ldl -lcrypt -lm -lsqlite3

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


.PHONY: aeaea
aeaea: $(BIN_DIR)/aeaea
$(BIN_DIR)/aeaea: $(O_FILES)
	rm -f $(BIN_DIR)/aeaea
	$(CC) -o $(BIN_DIR)/aeaea $(O_FILES) $(L_FLAGS)

.PHONY: tester
tester: C_FLAGS += -DTESTER
tester: aeaea

.PHONY: builder
builder: C_FLAGS += -DBUILDER
builder: aeaea

.PHONY: remort
remort: C_FLAGS += -DREMORT 
remort: aeaea

.PHONY: osx
osx: C_FLAGS = -I/usr/local/include -I/usr/local/Cellar/lua51/5.1.5_1/include/lua-5.1 -ggdb -w
osx: L_FLAGS = -L/usr/local/lib -ldl -llua5.1
osx: aeaea

# Compile live tests. The binary needs to be run rom area directory as with normal startup
.PHONY: livetest
livetest: $(BIN_DIR)/aeaea_livetest
$(BIN_DIR)/aeaea_livetest: C_FLAGS += -DLIVETEST
$(BIN_DIR)/aeaea_livetest: _live_tests $(O_FILES) $(LIVETEST_O_FILES) $(CU_TEST_O)
	rm -f $(BIN_DIR)/aeaea_livetest
	$(CC) -o $(BIN_DIR)/aeaea_livetest \
		$(O_FILES) \
		$(LIVETEST_O_FILES) \
		$(LIVETEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)

# Compile with unit tests and run immediately
.PHONY: unittest
unittest: $(BIN_DIR)/aeaea_unittest
$(BIN_DIR)/aeaea_unittest: C_FLAGS += -DUNITTEST
$(BIN_DIR)/aeaea_unittest: _unit_tests $(O_FILES) $(UNITTEST_O_FILES) $(CU_TEST_O)
	rm -f $(BIN_DIR)/aeaea_unittest
	$(CC) -o $(BIN_DIR)/aeaea_unittest \
		$(O_FILES) \
		$(UNITTEST_O_FILES) \
		$(UNITTEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)
	$(BIN_DIR)/aeaea_unittest

.PHONY: _live_tests
_live_tests: $(OBJ_DIR)
	./tests/make-live-tests.sh > ./tests/LiveTests.c
	$(CC) -c $(C_FLAGS) -o $(OBJ_DIR)/LiveTests.o ./tests/LiveTests.c

.PHONY: _unit_tests
_unit_tests: $(OBJ_DIR)
	./tests/make-unit-tests.sh > ./tests/UnitTests.c
	$(CC) -c $(C_FLAGS) -o $(OBJ_DIR)/UnitTests.o ./tests/UnitTests.c


$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o

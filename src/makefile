BIN_DIR := ../bin
OBJ_DIR := ../obj
VPATH = ./tests

CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2
MKTIME	:= \""$(shell date)"\"
BRANCH	:= \""$(shell hg branch)"\"
PARENT	:= \""$(shell hg summary | grep parent | sed 's/parent: //')"\"


MODE_FLAGS =

C_WARN = -Wall -Wextra -Wcast-align -Wdisabled-optimization -Winit-self -Wlogical-op \
	-Wstrict-prototypes -Wold-style-definition -Wno-unused-parameter -Wno-missing-field-initializers \
	-Wshadow

ifeq ($(C_WERROR),1)
	C_WARN += -Werror
endif

CPP_WARN = -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy \
	-Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations \
	-Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow \
	-Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 \
	-Wswitch-default -Wundef -Wno-unused-parameter -Weffc++ -Werror


C_FLAGS_BASE = -ggdb -rdynamic $(PROF) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
C_FLAGS_C = $(C_FLAGS_BASE) $(C_WARN) $(MODE_FLAGS)
C_FLAGS_CPP = $(C_FLAGS_BASE) $(CPP_WARN) $(MODE_FLAGS) -std=c++11

L_FLAGS =  $(PROF) -lstdc++ -llua -ldl -lcrypt -lm -lsqlite3

SRC_FILES_C := $(wildcard *.c) 
O_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES_C))
SRC_FILES_CPP := $(wildcard *.cpp)
OPP_FILES := $(patsubst %.cpp,$(OBJ_DIR)/%.opp,$(SRC_FILES_CPP))

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
$(BIN_DIR)/aeaea: $(OPP_FILES) $(O_FILES)
	rm -f $(BIN_DIR)/aeaea
	$(CC) -o $(BIN_DIR)/aeaea $(OPP_FILES) $(O_FILES) $(L_FLAGS)

.PHONY: tester
tester: MODE_FLAGS += -DTESTER
tester: aeaea

.PHONY: builder
builder: MODE_FLAGS += -DBUILDER
builder: aeaea

.PHONY: remort
remort: MODE_FLAGS += -DREMORT 
remort: aeaea

.PHONY: osx
osx: C_FLAGS_BASE = -I/usr/local/include -I/usr/local/Cellar/lua51/5.1.5_1/include/lua-5.1 -ggdb -w
osx: L_FLAGS = -L/usr/local/lib -ldl -llua5.1
osx: aeaea

.PHONY: lint
lint: lint_headers lint_src

.PHONY: lint_src
lint_src: $(addsuffix .lint,$(SRC_FILES_C))
lint_src: $(addsuffix .lint,$(SRC_FILES_CPP))

.PHONY: lint_headers
lint_headers: $(addsuffix .lint,$(wildcard *.h))
lint_headers: $(addsuffix .lint,$(wildcard *.hpp))

.PHONY: %.c.lint
%.c.lint: %.c
	$(CC) -c -fsyntax-only -x c $(C_WARN) $<

.PHONY: %.h.lint
%.h.lint: %.h
	$(CC) -c -fsyntax-only -x c $(C_WARN) $<

.PHONY: %.cpp.lint
%.cpp.lint: %.cpp
	$(CC) -c -fsyntax-only -x c++ $(CPP_WARN) $<

.PHONY: %.hpp.lint
%.hpp.lint: %.hpp
	$(CC) -c -fsyntax-only -x c++ $(CPP_WARN) $<


# Compile live tests. The binary needs to be run rom area directory as with normal startup
.PHONY: livetest
livetest: $(BIN_DIR)/aeaea_livetest
$(BIN_DIR)/aeaea_livetest: MODE_FLAGS += -DLIVETEST
$(BIN_DIR)/aeaea_livetest: _live_tests $(OPP_FILES) $(O_FILES) $(LIVETEST_O_FILES) $(CU_TEST_O)
	rm -f $(BIN_DIR)/aeaea_livetest
	$(CC) -o $(BIN_DIR)/aeaea_livetest \
		$(O_FILES) \
		$(OPP_FILES) \
		$(LIVETEST_O_FILES) \
		$(LIVETEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)

# Compile with unit tests and run immediately
.PHONY: unittest
unittest: $(BIN_DIR)/aeaea_unittest
$(BIN_DIR)/aeaea_unittest: MODE_FLAGS += -DUNITTEST
$(BIN_DIR)/aeaea_unittest: _unit_tests $(OPP_FILES) $(O_FILES) $(UNITTEST_O_FILES) $(CU_TEST_O)
	rm -f $(BIN_DIR)/aeaea_unittest
	$(CC) -o $(BIN_DIR)/aeaea_unittest \
		$(O_FILES) \
		$(OPP_FILES) \
		$(UNITTEST_O_FILES) \
		$(UNITTEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)
	$(BIN_DIR)/aeaea_unittest

.PHONY: _live_tests
_live_tests: $(OBJ_DIR)
	./tests/make-live-tests.sh > ./tests/LiveTests.c
	$(CC) -c $(C_FLAGS_C) -o $(OBJ_DIR)/LiveTests.o ./tests/LiveTests.c

.PHONY: _unit_tests
_unit_tests: $(OBJ_DIR)
	./tests/make-unit-tests.sh > ./tests/UnitTests.c
	$(CC) -c $(C_FLAGS_C) -o $(OBJ_DIR)/UnitTests.o ./tests/UnitTests.c

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_C) -o $@ $<

$(OBJ_DIR)/%.opp: %.cpp merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_CPP) -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.opp

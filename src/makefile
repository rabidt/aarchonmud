BIN_DIR := ../bin
OBJ_DIR := ../obj
TESTS_DIR := ./tests

CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2
MKTIME	:= \""$(shell date)"\"
BRANCH	:= \""$(shell hg branch)"\"
PARENT	:= \""$(shell hg summary | grep parent | sed 's/parent: //')"\"


MODE_FLAGS =

BASE_WARN = -Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Winit-self -Wlogical-op \
	-Wundef -Wno-unused-parameter -Wshadow -Wsign-conversion \
	-Wpointer-arith -Wuninitialized 


C_WARN = $(BASE_WARN) \
	-Wstrict-prototypes -Wold-style-definition -Wold-style-declaration \
	-Wno-missing-field-initializers -Waggregate-return
	
ifeq ($(C_WERROR),1)
	C_WARN += -Werror
endif

CPP_WARN = $(BASE_WARN) \
	-pedantic -Wctor-dtor-privacy -Wformat=2 -Wmissing-declarations -Wold-style-cast \
	-Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 \
	-Wswitch-default -Weffc++ -Werror

I_FLAGS_CPP = -I../3rdparty/rapidjson/include

C_FLAGS_BASE = -ggdb -rdynamic $(PROF) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
C_FLAGS_C = $(C_FLAGS_BASE) $(C_WARN) $(MODE_FLAGS)
C_FLAGS_CPP = $(C_FLAGS_BASE) $(CPP_WARN) $(I_FLAGS_CPP) $(MODE_FLAGS) -std=c++11

L_FLAGS =  $(PROF) -lstdc++ -llua -ldl -lcrypt -lm -lsqlite3

SRC_FILES_C := $(wildcard *.c) 
O_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES_C))
SRC_FILES_CPP := act_wiz.cpp changelog.cpp help.cpp perfmon.cpp str_buf.cpp
OPP_FILES := $(patsubst %.cpp,$(OBJ_DIR)/%.opp,$(SRC_FILES_CPP))

aeaea_OBJS := $(OPP_FILES) $(O_FILES)

.PHONY: all
all: aeaea

.PHONY: aeaea
aeaea: $(BIN_DIR)/aeaea
$(BIN_DIR)/aeaea: $(aeaea_OBJS)
	rm -f $(BIN_DIR)/aeaea
	$(CC) -o $(BIN_DIR)/aeaea $(aeaea_OBJS) $(L_FLAGS)

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

.PHONY: db_dump
db_dump: MODE_FLAGS += -DDB_DUMP
db_dump: $(BIN_DIR)/db_dump
$(BIN_DIR)/db_dump: $(OBJ_DIR)/db_dump.opp $(aeaea_OBJS)
	rm -f $(BIN_DIR)/db_dump
	$(CC) -o $(BIN_DIR)/db_dump $(OBJ_DIR)/db_dump.opp $(aeaea_OBJS) $(L_FLAGS)

.PHONY: lint
lint: lint_headers lint_src

.PHONY: lint_src
lint_src: $(addsuffix @lint,$(SRC_FILES_C))
lint_src: $(addsuffix @lint,$(SRC_FILES_CPP))

.PHONY: lint_headers
lint_headers: $(addsuffix @lint,$(wildcard *.h))
lint_headers: $(addsuffix @lint,$(wildcard *.hpp))

.PHONY: %.c@lint
%.c@lint: %.c
	$(CC) -c -fsyntax-only -x c $(C_WARN) $<

.PHONY: %.h@lint
%.h@lint: %.h
	$(CC) -c -fsyntax-only -x c $(C_WARN) $<

.PHONY: %.cpp@lint
%.cpp@lint: %.cpp
	$(CC) -c -fsyntax-only -x c++ $(CPP_WARN) $<

.PHONY: %.hpp@lint
%.hpp@lint: %.hpp
	$(CC) -c -fsyntax-only -x c++ $(CPP_WARN) $<


UNIT_SRCS_C := $(wildcard $(TESTS_DIR)/UNIT_*.c)
UNIT_OBJS_C := $(patsubst $(TESTS_DIR)/%.c,$(OBJ_DIR)/%.o,$(UNIT_SRCS_C))
unittest_OBJS := $(OBJ_DIR)/CuTest.o $(OBJ_DIR)/UnitTests.o $(UNIT_OBJS_C) $(aeaea_OBJS)

LIVE_SRCS_C := $(wildcard $(TESTS_DIR)/LIVE_*.c)
LIVE_OBJS_C := $(patsubst $(TESTS_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIVE_SRCS_C))
livetest_OBJS := $(OBJ_DIR)/CuTest.o $(OBJ_DIR)/LiveTests.o $(LIVE_OBJS_C) $(aeaea_OBJS)

# Compile live tests. The binary needs to be run rom area directory as with normal startup
.PHONY: livetest
livetest: $(BIN_DIR)/aeaea_livetest
$(BIN_DIR)/aeaea_livetest: MODE_FLAGS += -DLIVETEST
$(BIN_DIR)/aeaea_livetest: $(livetest_OBJS)
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

# Compile with unit tests and run immediately
.PHONY: unittest
unittest: $(BIN_DIR)/aeaea_unittest
$(BIN_DIR)/aeaea_unittest: MODE_FLAGS += -DUNITTEST
$(BIN_DIR)/aeaea_unittest: $(unittest_OBJS)
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)
	$@

$(TESTS_DIR)/LiveTests.c: $(LIVE_SRCS_C)
	$(TESTS_DIR)/make-live-tests.sh > $@

$(TESTS_DIR)/UnitTests.c: $(UNIT_SRCS_C)
	$(TESTS_DIR)/make-unit-tests.sh > $@


.PHONY: %.c@compile
%.c@compile: $(OBJ_DIR)/%.o ;

.PHONY: %.c@clean
%.c@clean:
	rm -f $(patsubst %.c@clean,$(OBJ_DIR)/%.o,$@)

.PHONY: %.cpp@compile
%.cpp@compile: $(OBJ_DIR)/%.opp ;

.PHONY: %.cpp@clean
%.cpp@clean:
	rm -f $(patsubst %.cpp@clean,$(OBJ_DIR)/%.opp,$@)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_C) -o $@ $<

$(OBJ_DIR)/%.o: tests/%.c | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_C) -o $@ $<

$(OBJ_DIR)/%.opp: %.cpp merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_CPP) -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.opp $(TESTS_DIR)/LiveTests.c $(TESTS_DIR)/UnitTests.c

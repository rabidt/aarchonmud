BIN_DIR := ../bin
OBJ_DIR := ../obj

CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2
MKTIME	:= \""$(shell date)"\"
BRANCH	:= \""$(shell git branch)"\"
PARENT	:= \""$(shell git rev-parse HEAD)"\"

# Any flags that need to be different on different platforms should be set here
# Default values should work for the main server and can be overriden from the environment in other cases as needed.
ifndef ARC_C_FLAGS
	ARC_C_FLAGS =
endif
ifndef ARC_L_FLAGS
	ARC_L_FLAGS = -llua
endif


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

C_FLAGS_BASE = -ggdb -rdynamic $(PROF) $(ARC_C_FLAGS) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
C_FLAGS_C = $(C_FLAGS_BASE) $(C_WARN) $(MODE_FLAGS)
C_FLAGS_CPP = $(C_FLAGS_BASE) $(CPP_WARN) $(I_FLAGS_CPP) $(MODE_FLAGS) -std=c++11

L_FLAGS =  $(PROF) -lstdc++ -ldl -lcrypt -lm -lsqlite3 $(ARC_L_FLAGS)

SRC_FILES_C := $(wildcard *.c) 
O_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES_C))
SRC_FILES_CPP := act_wiz.cpp changelog.cpp help.cpp perfmon.cpp str_buf.cpp boot_checks.cpp playback.cpp discord.cpp
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


CUTEST_TESTS_DIR := ./tests/CuTest
CUTEST_OBJ_DIR := $(OBJ_DIR)/CuTest
CUTEST_SRCS := $(wildcard $(CUTEST_TESTS_DIR)/UNIT_*.c)
CUTEST_OBJS := $(patsubst $(CUTEST_TESTS_DIR)/%.c,$(CUTEST_OBJ_DIR)/%.o,$(CUTEST_SRCS))
arc_cutest_OBJS := $(CUTEST_OBJ_DIR)/CuTest.o $(CUTEST_OBJ_DIR)/UnitTests.o $(CUTEST_OBJS) $(aeaea_OBJS)

CATCH2_TESTS_DIR := ./tests/catch2
CATCH2_OBJ_DIR := $(OBJ_DIR)/catch2
CATCH2_SRCS := $(wildcard $(CATCH2_TESTS_DIR)/*.cpp)
CATCH2_OBJS := $(patsubst $(CATCH2_TESTS_DIR)/%.cpp,$(CATCH2_OBJ_DIR)/%.opp,$(CATCH2_SRCS))
arc_catch2_OBJS := $(CATCH2_OBJS) $(aeaea_OBJS)

# Compile with unit tests and run immediately
.PHONY: arc_cutest
arc_cutest: $(BIN_DIR)/arc_cutest
$(BIN_DIR)/arc_cutest: MODE_FLAGS += -DARC_CUTEST
$(BIN_DIR)/arc_cutest: $(arc_cutest_OBJS)
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)
	$@

$(CUTEST_TESTS_DIR)/UnitTests.c: $(CUTEST_SRCS)
	$(CUTEST_TESTS_DIR)/make-unit-tests.sh > $@

.PHONY: arc_catch2
arc_catch2: $(BIN_DIR)/arc_catch2
$(BIN_DIR)/arc_catch2: MODE_FLAGS += -DARC_CATCH2
$(BIN_DIR)/arc_catch2: $(arc_catch2_OBJS)
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)
	TEST_BIN_PATH=`readlink -f $@` && cd ../data/area && $$TEST_BIN_PATH

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

$(OBJ_DIR) $(CATCH2_OBJ_DIR) $(CUTEST_OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: %.c merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_C) -o $@ $<

$(CUTEST_OBJ_DIR)/%.o: $(CUTEST_TESTS_DIR)/%.c | $(CUTEST_OBJ_DIR)
	$(CC) -c $(C_FLAGS_C) -o $@ $<

$(OBJ_DIR)/%.opp: %.cpp merc.h | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_CPP) -o $@ $<

$(CATCH2_OBJ_DIR)/%.opp: $(CATCH2_TESTS_DIR)/%.cpp merc.h | $(CATCH2_OBJ_DIR)
	$(CC) -c $(C_FLAGS_CPP) -I../3rdparty/catch2 -o $@ $<

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TESTS_DIR)/UnitTests.c

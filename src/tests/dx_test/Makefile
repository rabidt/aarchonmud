BIN_DIR := ../../../bin/dx_test
OBJ_DIR := ../../../obj/dx_test
VPATH = ut:../:../../
INC = -I../ -I../..


CC		:= gcc

C_WARN = -Wall -Wextra -Wcast-align -Wdisabled-optimization -Winit-self -Wlogical-op \
	-Wstrict-prototypes -Wold-style-definition -Wno-unused-parameter -Wno-missing-field-initializers
CPP_WARN = -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy \
	-Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations \
	-Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow \
	-Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 \
	-Wswitch-default -Wundef -Wno-unused-parameter -Weffc++ -Werror


C_FLAGS_BASE = -ggdb -rdynamic -O2 $(PROF) $(INC)
C_FLAGS_C = $(C_FLAGS_BASE) $(C_WARN)
C_FLAGS_CPP = $(C_FLAGS_BASE) $(CPP_WARN) -std=c++11

L_FLAGS =  $(PROF) -lstdc++ -ldl -lm -pthread


CU_TEST_O = $(OBJ_DIR)/CuTest.o
UT_OPP_FILES = $(patsubst ut/%.cpp,$(OBJ_DIR)/%.opp, \
	$(wildcard ut/*.cpp) )
UT_OPP_DEPS = $(patsubst %.cpp,$(OBJ_DIR)/%.opp, \
	dx_client.cpp dx_msg.cpp )

TEST_OPP_FILES = $(patsubst %.cpp,$(OBJ_DIR)/%.opp, \
	dx_test.cpp )
TEST_OPP_DEPS = $(patsubst %.cpp,$(OBJ_DIR)/%.opp, \
	dx_client.cpp dx_msg.cpp )


.PHONY: default
default: ut


.PHONY: all
all: ut test


.PHONY: ut
ut: $(BIN_DIR)/dx_ut
$(BIN_DIR)/dx_ut: _gen_ut $(UT_OPP_FILES) $(UT_OPP_DEPS) $(CU_TEST_O) | $(BIN_DIR)
	rm -f $(BIN_DIR)/dx_ut
	$(CC) -o $(BIN_DIR)/dx_ut \
		$(UT_OPP_FILES) \
		$(UT_OPP_DEPS) \
		$(CU_TEST_O) \
		$(L_FLAGS)
	$(BIN_DIR)/dx_ut

.PHONY: _gen_ut
_gen_ut:
	./ut/make-unit-tests.sh > ./ut/UnitTests.cpp


.PHONY: test
test: $(BIN_DIR)/dx_test
$(BIN_DIR)/dx_test: $(TEST_OPP_FILES) $(TEST_OPP_DEPS) | $(BIN_DIR)
	rm -f $(BIN_DIR)/dx_test
	$(CC) -o $(BIN_DIR)/dx_test \
		$(TEST_OPP_FILES) \
		$(TEST_OPP_DEPS) \
		$(L_FLAGS)


$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_C) -o $@ $<

$(OBJ_DIR)/%.opp: %.cpp | $(OBJ_DIR)
	$(CC) -c $(C_FLAGS_CPP) -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.opp
